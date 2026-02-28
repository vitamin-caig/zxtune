package app.zxtune.playback.service

import android.content.Context
import android.net.Uri
import android.os.Bundle
import androidx.core.net.toUri
import app.zxtune.Logger
import app.zxtune.TimeStamp
import app.zxtune.analytics.Analytics
import app.zxtune.core.Properties
import app.zxtune.device.sound.SoundOutputSamplesTarget
import app.zxtune.playback.Item
import app.zxtune.playback.PlayableItem
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.PlaybackControl.TrackMode
import app.zxtune.playback.PlaybackService
import app.zxtune.playback.SeekControl
import app.zxtune.playback.Visualizer
import app.zxtune.preferences.DataStore
import app.zxtune.sound.Player
import app.zxtune.sound.SamplesSource
import kotlinx.coroutines.CoroutineName
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.FlowPreview
import kotlinx.coroutines.Job
import kotlinx.coroutines.cancel
import kotlinx.coroutines.channels.ReceiveChannel
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.debounce
import kotlinx.coroutines.flow.distinctUntilChanged
import kotlinx.coroutines.flow.filterNotNull
import kotlinx.coroutines.flow.first
import kotlinx.coroutines.flow.getAndUpdate
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.merge
import kotlinx.coroutines.flow.runningFold
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.time.Duration.Companion.seconds

@OptIn(ExperimentalAtomicApi::class, FlowPreview::class)
class PlaybackServiceImpl(context: Context, private val prefs: DataStore) : PlaybackService {
    private val scope = CoroutineScope(Dispatchers.IO)
    private val queue = DispatcherQueue(context)
    private val player = Player.create(SoundOutputSamplesTarget.create(context))
    private val _current = MutableStateFlow<Holder?>(null)
    private val current
        get() = _current.value
    private val _activateJob = AtomicReference<Job?>(null)

    init {
        async("Collect queue stream") {
            queue.items.collect { items ->
                player.setSource(CompositeSource(items))
            }
        }
        async("Log errors") {
            player.errorsFlow.collect {
                LOG.w(it) { "Error occurred" }
            }
        }
        async("Session sync") {
            val storedSession = prefs.session?.apply {
                LOG.d { "Restore last played item $this" }
                activate(id) {
                    it.position = position
                }
            }
            merge(
                player.stateFlow, _current
            ).runningFold(storedSession) { session: Session?, update ->
                when (update) {
                    is Item -> Session(update.id, TimeStamp.EMPTY)
                    is Player.State.Stopped -> session?.copy(position = update.position)
                    else -> session
                }
            }.filterNotNull().distinctUntilChanged().debounce(10.seconds).collect { session ->
                LOG.d { "Store last played $session" }
                prefs.session = session
            }
        }
    }

    private fun activate(uri: Uri, onSuccess: suspend (Player) -> Unit) {
        _activateJob.exchange(scope.launch(CoroutineName("activate($uri)")) {
            val prev = current
            queue.activate(uri)
            // Do not match by url - real may differ from requested
            _current.first { it != prev }
            LOG.d { "Activated $uri as ${current?.id}" }
            onSuccess(player)
        })?.cancel()
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

    override fun setNowPlaying(uri: Uri) = activate(uri) {
        it.startPlayback()
    }

    override val state
        get() = player.stateFlow.map {
            when (it) {
                is Player.State.Started -> PlaybackControl.State.PLAYING to it.position
                is Player.State.Stopped -> PlaybackControl.State.STOPPED to it.position
                is Player.State.Finished -> PlaybackControl.State.STOPPED to it.position
                is Player.State.Seeking -> PlaybackControl.State.SEEKING to it.position
            }
        }

    override val nowPlaying: StateFlow<Item?>
        get() = _current

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
            next.exchange(null)?.let { next ->
                LOG.d { "Set current item to ${next.id}" }
                _current.getAndUpdate {
                    Holder(next, player.sampleRate)
                }?.release()
            }
            return current?.player
        }
    }

    companion object {
        private val LOG = Logger(PlaybackServiceImpl::class.java.name)
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
