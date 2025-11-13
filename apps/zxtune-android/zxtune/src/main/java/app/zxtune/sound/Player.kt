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
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.isActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.selects.select
import kotlinx.coroutines.withContext
import kotlinx.coroutines.yield
import java.util.concurrent.atomic.AtomicReference
import kotlin.time.Duration.Companion.seconds

class Player @VisibleForTesting constructor(
    private val target: SamplesTarget,
    private val events: PlayerEventsListener,
    private val renderDispatcher: CoroutineDispatcher,
    private val playDispatcher: CoroutineDispatcher,
) : Releaseable {
    private val errorHandler = CoroutineExceptionHandler { ctx, err ->
        LOG.w(err) {
            "Exception in ${ctx[CoroutineName]}"
        }
        ((err.cause ?: err) as? Exception)?.let { events.onError(it) }
    }
    private val scope = CoroutineScope(errorHandler)
    private val source = MutableStateFlow<SamplesSource>(StubSamplesSource)
    private val queue = Buffering(target.preferableBufferSize)
    private val seekRequest = AtomicReference<TimeStamp>(null)
    private val playbackPosition = AtomicReference(TimeStamp.EMPTY)
    private var job: Job? = null

    val sampleRate
        get() = target.sampleRate

    var position: TimeStamp
        get() = seekRequest.get() ?: playbackPosition.get()
        set(value) {
            seekRequest.set(value)
        }

    fun setSource(src: SamplesSource) {
        seekRequest.set(null)
        source.value = src
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
                events.onStart()
                var lastPosition: TimeStamp? = null
                while (isActive) {
                    if (maybeSeek(src)) {
                        lastPosition = null
                    }
                    queue.produce(src)?.let {
                        if (lastPosition != null && it <= lastPosition) {
                            LOG.d { "Loop after $lastPosition" }
                            events.onStart()
                        }
                        lastPosition = it
                        yield() // for collectLatest cancellation
                        continue
                    }
                    LOG.d { "Finished playback" }
                    events.onFinish()
                    break
                }
                // Wait for the next source
                delay(5.seconds)
                stopPlayback()
            }
        }.apply {
            invokeOnCompletion {
                LOG.d { "Stopped playback ($it)" }
                events.onStop()
            }
        }
        launch(CoroutineName("PlaySound") + playDispatcher) {
            target.start()
            while (isActive) {
                queue.consume { buf ->
                    playbackPosition.set(buf.position)
                    target.writeSamples(buf.sound)
                }
            }
        }.invokeOnCompletion {
            target.stop()
        }
    }

    private suspend fun maybeSeek(src: SamplesSource): Boolean {
        if (seekRequest.get() != null) {
            events.onSeeking()
            while (true) {
                val newPos = seekRequest.get()
                src.position = newPos
                if (seekRequest.compareAndSet(newPos, null)) {
                    // Mock new position till real playback happens
                    playbackPosition.set(newPos)
                    break
                }
                yield()
            }
            events.onStart()
            return true
        }
        return false
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

        // @return timestamp for successfully produced chunk
        suspend fun produce(src: SamplesSource) = producing.run {
            if (isFrom(src) || fill(src)) { // may throw in rare cases
                check(isFrom(src))
                producing = swap(this, ready, free)
                position
            } else {
                null
            }
        }

        suspend fun consume(block: (Chunk) -> Unit) = consuming.run {
            if (!isFree) {
                release()
                block(this)
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

        @JvmStatic
        fun create(target: SamplesTarget, events: PlayerEventsListener) =
            Player(target, events, Dispatchers.Default, Dispatchers.IO)
    }
}
