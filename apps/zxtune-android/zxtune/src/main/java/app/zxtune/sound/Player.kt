/**
 * @file
 * @brief Player implementation
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.sound

import androidx.annotation.VisibleForTesting
import app.zxtune.Logger
import app.zxtune.Releaseable
import app.zxtune.TimeStamp
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.CoroutineExceptionHandler
import kotlinx.coroutines.CoroutineName
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.NonCancellable
import kotlinx.coroutines.cancel
import kotlinx.coroutines.channels.BufferOverflow
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.asSharedFlow
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.getAndUpdate
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.selects.select
import kotlinx.coroutines.withContext
import kotlinx.coroutines.yield
import kotlin.concurrent.atomics.AtomicReference
import kotlin.concurrent.atomics.ExperimentalAtomicApi
import kotlin.time.Duration.Companion.seconds

@OptIn(ExperimentalAtomicApi::class)
class Player @VisibleForTesting constructor(
    private val target: SamplesTarget,
    private val renderDispatcher: CoroutineDispatcher,
    private val playDispatcher: CoroutineDispatcher,
) : Releaseable {
    private val errorHandler = CoroutineExceptionHandler { ctx, err ->
        LOG.w(err) {
            "Exception in ${ctx[CoroutineName]}"
        }
        ((err.cause ?: err) as? Exception)?.let { errors.tryEmit(it) }
    }
    private val scope = CoroutineScope(errorHandler)
    private val source = MutableStateFlow<SamplesSource>(StubSamplesSource)
    private val queue = Buffering(target.preferableBufferSize)
    private val seekRequest = AtomicReference<TimeStamp?>(null)
    private val playbackPosition = AtomicReference<TimeStamp?>(null)

    // Do not conflate similar states
    private val state = MutableSharedFlow<State>(
        replay = 1, extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_LATEST
    )
    private val errors = MutableSharedFlow<Exception>(
        replay = 0, extraBufferCapacity = 1, onBufferOverflow = BufferOverflow.DROP_LATEST
    )
    private var job: Job? = null

    val sampleRate
        get() = target.sampleRate

    var position: TimeStamp
        get() = seekRequest.load() ?: playbackPosition.load() ?: TimeStamp.EMPTY
        set(value) {
            seekRequest.store(value)
        }

    fun setSource(src: SamplesSource) {
        seekRequest.store(null)
        if (source.getAndUpdate { src } == StubSamplesSource) {
            runCatching {
                state.tryEmit(State.Stopped(src.position))
            }.onFailure { (it as? Exception)?.let(errors::tryEmit) }
        }
    }

    fun startPlayback() {
        if (!isStarted) {
            job = startJob()
        }
    }

    fun stopPlayback() {
        job?.cancel().also { job = null }
    }

    val isStarted
        get() = true == job?.isActive

    sealed interface State {
        data class Stopped(val position: TimeStamp = TimeStamp.EMPTY) : State
        data class Seeking(val position: TimeStamp = TimeStamp.EMPTY) : State
        data class Started(val position: TimeStamp = TimeStamp.EMPTY) : State
        data class Finished(val position: TimeStamp = TimeStamp.EMPTY) : State
    }

    val stateFlow: Flow<State>
        get() = state.asSharedFlow()

    val errorsFlow: Flow<Exception>
        get() = errors.asSharedFlow()

    override fun release() {
        scope.cancel()
        job?.run {
            runBlocking {
                join()
            }
        }.also {
            job = null
        }
        target.release()
    }

    private fun startJob() = scope.launch {
        launch(CoroutineName("RenderSound") + renderDispatcher) {
            source.collectLatest { src ->
                LOG.d { "Start playback of $src" }
                while (isActive) {
                    maybeSeek(src)
                    if (!queue.produce(src)) {
                        playbackPosition.load()?.let {
                            LOG.d { "Finished playback at $it" }
                            state.emit(State.Finished(it))
                        }
                        break
                    }
                }
                // Wait for the next source
                delay(5.seconds)
                stopPlayback()
            }
        }
        launch(CoroutineName("PlaySound") + playDispatcher) {
            target.start()
            var newStart = true
            while (isActive) {
                queue.consume { buf ->
                    val prev = playbackPosition.exchange(buf.position) ?: TimeStamp.EMPTY
                    if (newStart || prev >= buf.position) {
                        state.emit(State.Started(buf.position))
                        newStart = false
                    }
                    target.writeSamples(buf.sound)
                }
            }
        }.invokeOnCompletion {
            playbackPosition.load()?.let {
                LOG.d { "Stopped playback at $it" }
                state.tryEmit(State.Stopped(it))
            }
            target.stop()
        }
    }

    private suspend fun maybeSeek(src: SamplesSource) {
        while (true) {
            yield() // interruption point
            val newPos = seekRequest.load() ?: break
            state.emit(State.Seeking(newPos))
            src.position = newPos
            if (seekRequest.compareAndSet(newPos, null)) {
                playbackPosition.store(newPos)
                break
            }
        }
    }

    private class Chunk(var position: TimeStamp, val sound: ShortArray, var owner: Any?) {
        constructor(bufferSize: Int) : this(TimeStamp.EMPTY, ShortArray(bufferSize), null)

        fun fill(src: SamplesSource): Boolean {
            val pos = src.position
            return src.getSamples(sound).also {
                position = pos
                owner = if (it) src else null
            }
        }

        fun isFrom(src: SamplesSource) = owner === src

        val isFree
            get() = owner == null

        fun release() {
            owner = null
        }
    }

    private class Buffering(bufferSize: Int) {
        private val ready = Channel<Chunk>(Channel.RENDEZVOUS)
        private val free = Channel<Chunk>(Channel.RENDEZVOUS)
        private var producing = Chunk(bufferSize)
        private var consuming = Chunk(bufferSize)

        suspend fun produce(src: SamplesSource) = producing.run {
            if (isFrom(src) || fill(src)) { // may throw in rare cases
                check(isFrom(src))
                producing = swap(this, ready, free)
                true
            } else {
                false
            }
        }

        suspend fun consume(block: suspend (Chunk) -> Unit) = consuming.run {
            if (!isFree) {
                block(this)
                release()
            }
            consuming = swap(this, free, ready)
        }

        // Cannot use select for send/receive simultaneously on the same object
        private suspend fun swap(obj: Chunk, sendTo: Channel<Chunk>, recvFrom: Channel<Chunk>) =
            select {
                sendTo.onSend(obj) {
                    withContext(NonCancellable) {
                        recvFrom.receive()
                    }
                }
                recvFrom.onReceive { output ->
                    withContext(NonCancellable) {
                        sendTo.send(obj)
                    }
                    output
                }
            }
    }

    companion object {
        private val LOG = Logger(Player::class.java.name)

        fun create(target: SamplesTarget) = Player(target, Dispatchers.Default, Dispatchers.IO)
    }
}
