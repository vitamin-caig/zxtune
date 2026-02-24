package app.zxtune.playback.service

import android.content.Context
import android.net.Uri
import android.os.Bundle
import androidx.annotation.VisibleForTesting
import androidx.core.net.toUri
import app.zxtune.Logger
import app.zxtune.TimeStamp
import app.zxtune.analytics.Analytics
import app.zxtune.core.Properties
import app.zxtune.device.sound.SoundOutputSamplesTarget
import app.zxtune.playback.Item
import app.zxtune.playback.PlayableItem
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.PlaybackService
import app.zxtune.playback.SeekControl
import app.zxtune.playback.Visualizer
import app.zxtune.preferences.DataStore
import app.zxtune.sound.Player
import app.zxtune.sound.SamplesSource
import kotlinx.coroutines.CoroutineDispatcher
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
import kotlinx.coroutines.flow.launchIn
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.merge
import kotlinx.coroutines.flow.onEach
import kotlinx.coroutines.flow.runningFold
import kotlinx.coroutines.launch
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.time.Duration.Companion.seconds

@OptIn(ExperimentalAtomicApi::class, FlowPreview::class)
class PlaybackServiceImpl
@VisibleForTesting constructor(
    private val queue: Queue,
    private val player: Player,
    private val prefs: DataStore,
    dispatcher: CoroutineDispatcher
) : PlaybackService {
    private val scope = CoroutineScope(dispatcher)
    private val _current = MutableStateFlow<Holder?>(null)
    private val current
        get() = _current.value
    private val _activateJob = AtomicReference<Job?>(null)

    constructor(context: Context, prefs: DataStore) : this(
        DispatcherQueue(context),
        Player.create(SoundOutputSamplesTarget.create(context)),
        prefs,
        Dispatchers.IO
    )

    init {
        queue.items.onEach { items ->
            player.setSource(CompositeSource(items).apply {
                tryFetchNext()
            })
        }.launchIn(scope)
        player.errorsFlow.onEach {
            LOG.w(it) { "Error occurred" }
        }.launchIn(scope)
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

    override val playbackControl = object : PlaybackControl {
        private val _looped = MutableStateFlow(false)
        private val _shuffled = MutableStateFlow(false)

        init {
            async("PrefsSync") {
                _looped.apply { value = prefs.isLooped }.onEach {
                    prefs.isLooped = it
                    LOG.d { "Looped=$it" }
                }.launchIn(scope)
                _shuffled.apply { value = prefs.isShuffled }.onEach {
                    prefs.isShuffled = it
                    queue.shuffled = it
                    LOG.d { "Shuffled=$it" }
                }.launchIn(scope)
            }
        }

        override fun play() = player.startPlayback()
        override fun stop() = player.stopPlayback()
        override fun next() = async("navigateToNext") {
            queue.next()
        }

        override fun prev() = async("navigateToPrev") {
            queue.prev()
        }

        override var trackLooped by _looped::value
        override var shuffledOrder by _shuffled::value
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
                is Player.State.Started -> PlaybackService.State.PLAYING to it.position
                is Player.State.Stopped -> PlaybackService.State.STOPPED to it.position
                is Player.State.Finished -> PlaybackService.State.STOPPED to it.position
                is Player.State.Seeking -> PlaybackService.State.SEEKING to it.position
            }
        }

    override val nowPlaying: StateFlow<Item?>
        get() = _current

    private class Holder(delegate: PlayableItem, sampleRate: Int) : PlayableItem by delegate {
        val player = module.createPlayer(sampleRate)

        fun release() {
            Analytics.sendPlayEvent(this, player)
            player.release()
            module.release()
        }
    }

    // TODO: add SamplesSource.Release
    private inner class CompositeSource(val stream: ReceiveChannel<PlayableItem>) : SamplesSource {
        suspend fun tryFetchNext() = stream.receiveCatching().getOrNull()?.let { next ->
            LOG.d { "Set current item to ${next.id}" }
            _current.getAndUpdate { Holder(next, player.sampleRate) }?.release()
        } ?: Unit

        override fun toString() = "CompositeSource(start=${current?.id})"

        override suspend fun getSamples(buf: ShortArray): Boolean {
            while (true) {
                renderer?.run {
                    if (render(buf)) {
                        return true
                    }
                    position = TimeStamp.EMPTY
                    tryFetchNext()
                } ?: return false
            }
        }

        override var position
            get() = renderer?.position ?: TimeStamp.EMPTY
            set(value) = renderer?.run {
                position = value
            } ?: Unit

        private val renderer
            get() = current?.player
    }

    companion object {
        private val LOG = Logger(PlaybackServiceImpl::class.java.name)
        private const val PREF_LAST_PLAYED_PATH = "last_played_path"
        private const val PREF_LAST_PLAYED_POSITION = "last_played_position"
        private const val PREF_SHUFFLED_PLAYBACK = "playback.shuffled"

        private data class Session(val id: Uri, val position: TimeStamp) {
            override fun toString() = "$id@$position"
        }

        // Operate only on IO thread (i.e. |scope|-controlled jobs)
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
