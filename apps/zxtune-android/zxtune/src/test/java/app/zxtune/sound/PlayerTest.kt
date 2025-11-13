package app.zxtune.sound

import app.zxtune.TestUtils.flushEvents
import app.zxtune.TimeStamp
import app.zxtune.core.jni.JniRuntimeException
import app.zxtune.use
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.doThrow
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.stub
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner
import java.util.concurrent.atomic.AtomicInteger
import kotlin.time.Duration.Companion.minutes

@RunWith(RobolectricTestRunner::class)
class PlayerTest {
    private val dispatcher = StandardTestDispatcher()

    private val sourceChunkIndex = AtomicInteger(0)
    private val renderTimestamp
        get() = TimeStamp.fromSeconds(sourceChunkIndex.toLong())

    private fun fillNextChunk(buf: ShortArray) = sourceChunkIndex.incrementAndGet().also {
        buf.fill(it.toShort())
    }

    private val source = mock<SamplesSource> {
        on { position } doAnswer { renderTimestamp }
        on { getSamples(any()) } doAnswer {
            fillNextChunk(it.getArgument(0))
            true
        }
    }

    private val chunksFromSource2 = 4
    private val source2 = mock<SamplesSource>() {
        var doneChunks = 0

        on { position } doAnswer {
            TimeStamp.fromSeconds(doneChunks.toLong())
        }
        on { getSamples(any()) } doAnswer {
            if (++doneChunks > chunksFromSource2) {
                false
            } else {
                fillNextChunk(it.getArgument(0))
                true
            }
        }
    }
    private val targetChunkIndex = AtomicInteger(0)
    private val target = mock<SamplesTarget> {
        on { preferableBufferSize } doReturn 1
        on { writeSamples(any()) } doAnswer {
            val buffer = it.getArgument<ShortArray>(0)
            val buf = buffer[0].toInt()
            assertTrue(targetChunkIndex.compareAndSet(buf - 1, buf))
        }
    }
    private val events = mock<PlayerEventsListener>()
    private val underTest = Player(target, events, dispatcher, dispatcher)

    @After
    fun tearDown() = verifyNoMoreInteractions(source, source2, target, events)

    @Test
    fun `no-op workflow`() {
        underTest.use {
            assertFalse(isStarted)
        }
        verify(target).preferableBufferSize
        verify(target).release()
    }

    @Test
    fun `no source workflow`() {
        underTest.use {
            runTest(dispatcher) {
                assertFalse(isStarted)
                startPlayback()
                assertTrue(isStarted)
                while (isStarted) {
                    flushEvents()
                }
                assertFalse(isStarted)
            }
        }

        inOrder(source, target, events) {
            verify(target).preferableBufferSize
            verify(target).start()
            verify(events).onStart()
            verify(events).onFinish()
            verify(target).stop()
            verify(events).onStop()
            verify(target).release()
        }
    }

    @Test
    fun `failed source getSamples`() {
        val error = JniRuntimeException("Unused")
        source.stub {
            on { getSamples(any()) } doThrow error
        }
        executeTest()

        inOrder(source, target, events) {
            verify(target).preferableBufferSize
            verify(target).start()
            verify(events).onStart()
            verify(source).position
            verify(source).getSamples(any())
            verify(events).onStop()
            verify(target).stop()
            verify(events).onError(error)
            verify(target).release()
        }
    }

    @Test
    fun `failed source getPosition`() {
        val error = JniRuntimeException("Unused")
        source.stub {
            on { position } doThrow error
        }
        executeTest()

        inOrder(source, target, events) {
            verify(target).preferableBufferSize
            verify(target).start()
            verify(events).onStart()
            verify(source).position
            verify(events).onStop()
            verify(target).stop()
            verify(events).onError(error)
            verify(target).release()
        }
    }

    @Test
    fun `failed target start`() {
        val error = RuntimeException()
        target.stub {
            on { start() } doThrow error
        }
        executeTest()

        inOrder(source, target, events) {
            verify(target).preferableBufferSize
            verify(target).start()
            verify(target).stop()
            verify(events).onStop()
            verify(events).onError(error)
            verify(target).release()
        }
    }

    @Test
    fun `failed target writeSamples`() {
        val error = RuntimeException()
        target.stub {
            on { writeSamples(any()) } doThrow error
        }
        executeTest()

        inOrder(source, target, events) {
            verify(target).preferableBufferSize
            verify(target).start()
            verify(events).onStart()
            verify(source).position
            verify(source).getSamples(any())
            verify(target).writeSamples(any())

            verify(target).stop()
            verify(events).onStop()
            verify(events).onError(error)
            verify(target).release()
        }
    }

    @Test
    fun `failed target stop`() {
        val error = RuntimeException()
        source.stub {
            on { getSamples(any()) } doReturn false
        }
        target.stub {
            on { stop() } doThrow error
        }
        executeTest()

        inOrder(source, target, events) {
            verify(target).preferableBufferSize
            verify(target).start()
            verify(events).onStart()
            verify(source).position
            verify(source).getSamples(any())
            verify(events).onFinish()
            verify(target).stop()
            verify(events).onError(error)
            verify(events).onStop()
            verify(target).release()
        }
    }

    @Test
    fun `chained sources workflow`() {
        val chunksFromSource = 2
        source.stub {
            on { getSamples(any()) } doAnswer {
                fillNextChunk(it.getArgument(0)) <= chunksFromSource
            }
        }
        events.stub {
            on { onFinish() } doAnswer {
                // new sequence
                sourceChunkIndex.set(100)
                targetChunkIndex.set(100)
                underTest.setSource(source2)
            }
        }
        executeTest()

        inOrder(source, source2, target, events) {
            verify(target).preferableBufferSize
            verify(target).start()
            // source
            verify(events).onStart()
            repeat(chunksFromSource) {
                verify(source).position
                verify(source).getSamples(any())
                verify(target).writeSamples(any())
            }
            verify(source).position
            verify(source).getSamples(any())
            verify(events).onFinish()
            // source2
            verify(events).onStart()
            repeat(chunksFromSource2) {
                verify(source2).position
                verify(source2).getSamples(any())
                verify(target).writeSamples(any())
            }
            verify(source2).position
            verify(source2).getSamples(any())
            verify(events).onFinish()
            // timeout here
            verify(target).stop()
            verify(events).onStop()
            verify(target).release()
        }
    }

    @Test
    fun `stop and resume playback`() {
        val chunksBeforeStop = 3
        source.stub {
            on { getSamples(any()) } doAnswer {
                if (0 == (fillNextChunk(it.getArgument(0)) % chunksBeforeStop)) {
                    underTest.stopPlayback()
                }
                true
            }
        }
        executeTest(2)

        inOrder(source, target, events) {
            verify(target).preferableBufferSize

            // pass1
            verify(target).start()
            verify(events).onStart()
            // chunks 1&2
            repeat(chunksBeforeStop - 1) {
                verify(source).position
                verify(source).getSamples(any())
                verify(target).writeSamples(any())
            }
            //chunk 3 not written
            verify(source).position
            verify(source).getSamples(any())

            verify(events).onStop()
            verify(target).stop()

            // pass2
            verify(target).start()
            verify(events).onStart()
            // chunks 3,4,5. chunk 6 is not written
            repeat(chunksBeforeStop) {
                verify(target).writeSamples(any())
                verify(source).position
                verify(source).getSamples(any())
            }
            verify(events).onStop()
            verify(target).stop()
            verify(target).release()
        }
    }

    @Test
    fun `stop and change source`() {
        val chunksFromSource = 3
        val chunksFromSource2 = 4
        source.stub {
            var doneChunks = 0

            on { getSamples(any()) } doAnswer {
                if (++doneChunks > chunksFromSource) {
                    underTest.stopPlayback()
                }
                fillNextChunk(it.getArgument(0))
                true
            }
        }
        events.stub {
            on { onStop() } doAnswer {
                // new sequence
                sourceChunkIndex.set(100)
                targetChunkIndex.set(100)
                underTest.setSource(source2)
            }
        }

        executeTest(2)

        inOrder(source, source2, target, events) {
            verify(target).preferableBufferSize

            // pass1
            verify(target).start()
            verify(events).onStart()
            // chunks 1&2&3
            repeat(chunksFromSource) {
                verify(source).position
                verify(source).getSamples(any())
                verify(target).writeSamples(any())
            }
            //chunk 4 not written
            verify(source).position
            verify(source).getSamples(any())

            verify(events).onStop()
            verify(target).stop()

            // pass2
            verify(target).start()
            verify(events).onStart()
            repeat(chunksFromSource2) {
                verify(source2).position
                verify(source2).getSamples(any())
                verify(target).writeSamples(any())
            }
            verify(source2).position
            verify(source2).getSamples(any())

            verify(events).onFinish()
            verify(target).stop()
            verify(events).onStop()
            verify(target).release()
        }
    }

    @Test
    fun `set source on the fly`() {
        val chunksFromSource = 3
        source.stub {
            var doneChunks = 0

            on { getSamples(any()) } doAnswer {
                assert(sourceChunkIndex.get() < 100)
                fillNextChunk(it.getArgument(0))
                if (++doneChunks == chunksFromSource) {
                    underTest.setSource(source2)
                }
                true
            }
        }
        var startIndex = 0
        events.stub {
            on { onStart() } doAnswer {
                sourceChunkIndex.set(startIndex)
                targetChunkIndex.set(startIndex)
                startIndex += 100
            }
        }

        executeTest()

        inOrder(source, source2, target, events) {
            verify(target).preferableBufferSize

            // pass1
            verify(target).start()
            verify(events).onStart()
            repeat(chunksFromSource) {
                verify(source).position
                verify(source).getSamples(any())
                verify(target).writeSamples(any())
            }
            verify(events).onStart()
            repeat(chunksFromSource2) {
                verify(source2).position
                verify(source2).getSamples(any())
                verify(target).writeSamples(any())
            }
            verify(source2).position
            verify(source2).getSamples(any())

            verify(events).onFinish()
            verify(target).stop()
            verify(events).onStop()
            verify(target).release()
        }
    }

    @Test
    fun `looped playback events`() {
        val chunksInTrack = 3
        val chunksFromSource = 10
        source.stub {
            on { position } doAnswer { TimeStamp.fromSeconds(sourceChunkIndex.toLong() % chunksInTrack) }

            on { getSamples(any()) } doAnswer {
                fillNextChunk(it.getArgument(0)) <= chunksFromSource
            }
        }

        executeTest()

        inOrder(source, target, events) {
            verify(target).preferableBufferSize

            verify(target).start()
            verify(events).onStart()
            repeat(chunksFromSource) { idx ->
                verify(source).position
                verify(source).getSamples(any())
                verify(target).writeSamples(any())
                if (idx > 0 && 0 == (idx % chunksInTrack)) {
                    verify(events).onStart()
                }
            }
            verify(source).position
            verify(source).getSamples(any())

            verify(events).onFinish()
            verify(target).stop()
            verify(events).onStop()
            verify(target).release()
        }
    }

    @Test
    fun `seek playback events`() {
        val chunksFromSource = 10
        source.stub {
            var pos = 0
            on { position } doAnswer { TimeStamp.fromSeconds(pos.toLong()) }
            on { position = any() } doAnswer {
                pos = it.getArgument<TimeStamp>(0).toSeconds().toInt()
            }

            on { getSamples(any()) } doAnswer {
                ++pos
                val done = fillNextChunk(it.getArgument(0))
                if (done == 5) {
                    underTest.position = TimeStamp.fromSeconds(3)
                }
                done <= chunksFromSource
            }
        }

        executeTest()

        inOrder(source, target, events) {
            verify(target).preferableBufferSize

            verify(target).start()
            verify(events).onStart()
            repeat(5) {
                verify(source).position
                verify(source).getSamples(any())
                verify(target).writeSamples(any())
            }
            verify(events).onSeeking()
            verify(source).position = any()
            verify(events).onStart()
            repeat(chunksFromSource - 5) {
                verify(source).position
                verify(source).getSamples(any())
                verify(target).writeSamples(any())
            }
            verify(source).position
            verify(source).getSamples(any())

            verify(events).onFinish()
            verify(target).stop()
            verify(events).onStop()
            verify(target).release()
        }
    }

    private fun executeTest(repeats: Int = 1) = underTest.use {
        runTest(dispatcher, timeout = 1.minutes) {
            assertFalse(isStarted)
            setSource(source)
            repeat(repeats) {
                assertFalse(isStarted)
                startPlayback()
                assertTrue(isStarted)
                while (isStarted) {
                    flushEvents()
                }
                assertFalse(isStarted)
            }
        }
    }
}
