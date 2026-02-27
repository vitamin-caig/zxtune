package app.zxtune.playback.service

import android.content.Context
import android.net.Uri
import android.os.Bundle
import androidx.core.net.toUri
import app.zxtune.Logger
import app.zxtune.Releaseable
import app.zxtune.TimeStamp
import app.zxtune.analytics.Analytics
import app.zxtune.core.Properties
import app.zxtune.device.sound.SoundOutputSamplesTarget
import app.zxtune.playback.Callback
import app.zxtune.playback.CompositeCallback
import app.zxtune.playback.DispatcherQueue
import app.zxtune.playback.Item
import app.zxtune.playback.PlayableItem
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.PlaybackControl.TrackMode
import app.zxtune.playback.PlaybackService
import app.zxtune.playback.SeekControl
import app.zxtune.playback.Visualizer
import app.zxtune.playback.stubs.PlayableItemStub
import app.zxtune.preferences.DataStore
import app.zxtune.sound.Player
import app.zxtune.sound.SamplesSource
import kotlinx.coroutines.CoroutineName
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.CoroutineStart
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.cancel
import kotlinx.coroutines.channels.ReceiveChannel
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi

@OptIn(ExperimentalAtomicApi::class)
class PlaybackServiceImpl(context: Context, private val prefs: DataStore) : PlaybackService {
    private val scope = CoroutineScope(Dispatchers.IO)
    private val queue = DispatcherQueue(context)
    private val callbacks = CompositeCallback().apply {
        onInitialState(PlaybackControl.State.STOPPED)
    }
    private val player = Player.create(SoundOutputSamplesTarget.create(context))
    private val _current = AtomicReference<Holder?>(null)
    private val current
        get() = _current.load()
    private val sessionRestoring =
        scope.launch(CoroutineName("sessionRestore"), CoroutineStart.LAZY) {
            prefs.session?.run {
                LOG.d { "Restore last played item $this" }
                queue.activate(id)
                player.position = position
            }
        }

    init {
        async("Collect queue stream") {
            queue.items.collect { items ->
                player.setSource(CompositeSource(items))
            }
        }
        async("Translate state") {
            player.stateFlow.collect { state ->
                when (state) {
                    is Player.State.Started -> onStateChanged(
                        PlaybackControl.State.PLAYING, state.position
                    )

                    is Player.State.Stopped -> onStateChanged(
                        PlaybackControl.State.STOPPED, state.position
                    )

                    is Player.State.Seeking -> onStateChanged(
                        PlaybackControl.State.SEEKING, state.position
                    )

                    else -> Unit
                }
            }
        }
        async("Log errors") {
            player.errorsFlow.collect {
                LOG.w(it) { "Error occurred" }
            }
        }
        callbacks.add(object : Callback {
            private var session: Session? = null

            override fun onInitialState(state: PlaybackControl.State) = Unit
            override fun onStateChanged(
                state: PlaybackControl.State, pos: TimeStamp
            ) {
                pos.takeIf { state == PlaybackControl.State.STOPPED }?.let {
                    session?.copy(position = it)?.let { session ->
                        async("storeSession") {
                            LOG.d { "Store last played $session" }
                            prefs.session = session
                        }
                    } ?: LOG.d { "No current session" }
                }
            }

            override fun onItemChanged(item: Item) {
                session = Session(item.id, TimeStamp.EMPTY)
            }

            override fun onError(e: String) = Unit
        })
    }

    private fun async(name: String, block: suspend CoroutineScope.() -> Unit) {
        scope.launch(CoroutineName(name), block = block)
    }

    override fun release() {
        scope.cancel()
        player.release()
        current?.release()
    }

    override val playbackControl: PlaybackControl = object : PlaybackControl {
        private var _shuffled
            get() = queue.shuffled
            set(value) {
                if (value != queue.shuffled) {
                    queue.shuffled = value
                    prefs.isShuffled = value
                }
            }
        private var _looped = prefs.isLooped
            set(value) {
                if (value != field) {
                    field = value
                    prefs.isLooped = value
                }
            }

        init {
            _shuffled = prefs.isShuffled
        }

        override fun play() = player.startPlayback()
        override fun stop() = player.stopPlayback()
        override fun next() = async("navigateToNext") {
            queue.next()
        }

        override fun prev() = async("navigateToPrev") {
            queue.prev()
        }

        override var trackMode
            get() = if (_looped) TrackMode.LOOPED else TrackMode.REGULAR
            set(value) = when (value) {
                TrackMode.LOOPED -> _looped = true
                TrackMode.REGULAR -> _looped = false
            }
        override var sequenceMode
            get() = if (_shuffled) PlaybackControl.SequenceMode.SHUFFLE else PlaybackControl.SequenceMode.ORDERED
            set(value) = when (value) {
                PlaybackControl.SequenceMode.SHUFFLE -> _shuffled = true
                PlaybackControl.SequenceMode.ORDERED -> _shuffled = false
                else -> Unit
            }
    }
    override val seekControl = object : SeekControl {
        override val duration
            get() = current?.duration ?: TimeStamp.EMPTY
        override var position by player::position

    }
    override val visualizer = object : Visualizer {
        override fun getSpectrum(levels: ByteArray) = current?.player?.analyze(levels) ?: 0
    }

    override val playbackProperties
        get() = current?.player

    override val nowPlaying
        get() = current ?: PlayableItemStub

    override fun setNowPlaying(uri: Uri) = async("setNowPlaying") {
        sessionRestoring.cancel()
        queue.activate(uri)
        player.startPlayback()
    }

    override fun restoreSession() {
        sessionRestoring.start()
    }

    override fun subscribe(cb: Callback): Releaseable = callbacks.add(cb)

    private open class Holder(delegate: PlayableItem, sampleRate: Int) : PlayableItem by delegate {
        val player = module.createPlayer(sampleRate)

        fun release() {
            Analytics.sendPlayEvent(this, player)
            player.release()
            module.release()
        }
    }

    // TODO: add SamplesSource.Release
    private inner class CompositeSource(val stream: ReceiveChannel<PlayableItem>) : SamplesSource {
        private val next = AtomicReference<PlayableItem?>(null)

        init {
            readNext()
            dispatchPlayer()
        }

        override fun toString() = "CompositeSource(start=${_current.value?.id})"

        private fun readNext() = runBlocking {
            next.store(stream.receiveCatching().getOrNull())
        }

        override fun getSamples(buf: ShortArray): Boolean {
            while (true) {
                val renderer = dispatchPlayer() ?: return false
                if (renderer.render(buf)) {
                    return true
                }
                renderer.position = TimeStamp.EMPTY
                readNext()
            }
        }

        override var position
            get() = dispatchPlayer()?.position ?: TimeStamp.EMPTY
            set(value) = dispatchPlayer()?.run {
                position = value
            } ?: Unit

        private fun dispatchPlayer(): app.zxtune.core.Player? {
            next.exchange(null)?.let {
                LOG.d { "Set current item to ${it.id}" }
                _current.exchange(Holder(it, player.sampleRate))?.release()
                callbacks.onItemChanged(it)
            }
            return current?.player
        }
    }

    private fun onStateChanged(state: PlaybackControl.State, position: TimeStamp) =
        callbacks.onStateChanged(state, position)

    companion object {
        private val LOG = Logger("PlaybackService")
        private const val PREF_LAST_PLAYED_PATH = "last_played_path"
        private const val PREF_LAST_PLAYED_POSITION = "last_played_position"
        private const val PREF_SHUFFLED_PLAYBACK = "playback.shuffled"

        private data class Session(val id: Uri, val position: TimeStamp) {
            override fun toString() = "$id@$position"
        }

        private var DataStore.session
            get() = getString(PREF_LAST_PLAYED_PATH, null)?.let { path ->
                Session(
                    path.toUri(), TimeStamp.fromMilliseconds(getLong(PREF_LAST_PLAYED_POSITION, 0))
                )
            }
            set(value) = value?.run {
                Bundle().apply {
                    putString(PREF_LAST_PLAYED_PATH, id.toString())
                    putLong(PREF_LAST_PLAYED_POSITION, position.toMilliseconds())
                }.let {
                    putBatch(it)
                }
            } ?: Unit

        private var DataStore.isLooped
            get() = getLong(Properties.Sound.LOOPED, 0) != 0L
            set(value) = putLong(Properties.Sound.LOOPED, if (value) 1 else 0)

        private var DataStore.isShuffled
            get() = getLong(PREF_SHUFFLED_PLAYBACK, 0) != 0L
            set(value) = putLong(PREF_SHUFFLED_PLAYBACK, if (value) 1 else 0)
    }
}
