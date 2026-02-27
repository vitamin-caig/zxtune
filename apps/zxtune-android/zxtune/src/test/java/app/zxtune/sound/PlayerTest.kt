package app.zxtune.sound

import app.zxtune.TestUtils.flushEvents
import app.zxtune.TestUtils.mockCollectorOf
import app.zxtune.TimeStamp
import app.zxtune.core.jni.JniRuntimeException
import app.zxtune.use
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.TestScope
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.argThat
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

@RunWith(RobolectricTestRunner::class)
class PlayerTest {
    private val dispatcher = StandardTestDispatcher()
    private val scope = TestScope(dispatcher)

    private val sourceChunkIndex = AtomicInteger(0)
    private val renderTimestamp
        get() = sourceChunkIndex.get().toTimestamp()

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
    private val source2 = mock<SamplesSource> {
        var doneChunks = 0

        on { position } doAnswer {
            doneChunks.toTimestamp()
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
    private val underTest = Player(target, dispatcher, dispatcher)
    private val states = scope.mockCollectorOf(underTest.stateFlow)
    private val errors = scope.mockCollectorOf(underTest.errorsFlow)

    @After
    fun tearDown() = verifyNoMoreInteractions(source, source2, target, states, errors)

    @Test
    fun `no-op workflow`() = scope.runTest {
        underTest.use {
            assertFalse(isStarted)
        }
        verify(target).preferableBufferSize
        verify(target).release()
    }

    @Test
    fun `no source workflow`() = scope.runTest {
        underTest.use {
            assertFalse(isStarted)
            startPlayback()
            assertTrue(isStarted)
            while (isStarted) {
                flushEvents()
            }
            assertFalse(isStarted)
        }

        inOrder(source, target, states) {
            verify(target).preferableBufferSize
            verify(target).start()
            verify(target).stop()
            verify(target).release()
        }
    }

    @Test
    fun `failed source getSamples`() = scope.runTest {
        val error = JniRuntimeException("Unused")
        val initialPosition = 5.toTimestamp()
        source.stub {
            on { position } doReturn initialPosition
            on { getSamples(any()) } doThrow error
        }
        executeTest()

        inOrder(source, target, states, errors) {
            verify(target).preferableBufferSize
            verify(source).position
            verify(states).invoke(Player.State.Stopped(initialPosition))

            verify(target).start()
            verify(source).position
            verify(source).getSamples(any())
            verify(target).stop()
            verify(errors).invoke(error)
            verify(target).release()
        }
    }

    @Test
    fun `failed source getPosition`() = scope.runTest {
        val error = JniRuntimeException("Unused")
        source.stub {
            on { position } doThrow error
        }
        executeTest()

        inOrder(source, target, states, errors) {
            verify(target).preferableBufferSize
            verify(source).position
            verify(errors).invoke(error)
            verify(target).start()
            verify(source).position
            verify(target).stop()
            verify(errors).invoke(error)
            verify(target).release()
        }
    }

    @Test
    fun `failed target start`() = scope.runTest {
        val error = RuntimeException()
        target.stub {
            on { start() } doThrow error
        }
        executeTest()

        inOrder(source, target, states, errors) {
            verify(target).preferableBufferSize
            verify(source).position
            verify(states).invoke(Player.State.Stopped())

            verify(target).start()
            verify(target).stop()
            verify(errors).invoke(error)
            verify(target).release()
        }
    }

    @Test
    fun `failed target writeSamples`() = scope.runTest {
        val error = RuntimeException()
        target.stub {
            on { writeSamples(any()) } doThrow error
        }
        executeTest()

        inOrder(source, target, states, errors) {
            verify(target).preferableBufferSize
            verify(source).position
            verify(states).invoke(Player.State.Stopped())

            verify(target).start()
            verify(source).position
            verify(source).getSamples(any())
            verify(states).invoke(Player.State.Started())
            verify(target).writeSamples(any())

            verify(states).invoke(Player.State.Stopped())
            verify(target).stop()
            verify(errors).invoke(error)
            verify(target).release()
        }
    }

    @Test
    fun `failed target stop`() = scope.runTest {
        val error = RuntimeException()
        source.stub {
            on { getSamples(any()) } doReturn false
        }
        target.stub {
            on { stop() } doThrow error
        }
        executeTest()

        inOrder(source, target, states, errors) {
            verify(target).preferableBufferSize
            verify(source).position
            verify(states).invoke(Player.State.Stopped())

            verify(target).start()
            verify(source).position
            verify(source).getSamples(any())
            verify(target).stop()
            verify(errors).invoke(error)
            verify(target).release()
        }
    }

    @Test
    fun `chained sources workflow`() = scope.runTest {
        val chunksFromSource = 2
        source.stub {
            on { getSamples(any()) } doAnswer {
                fillNextChunk(it.getArgument(0)) <= chunksFromSource
            }
        }
        states.stub {
            on { invoke(argThat { this is Player.State.Finished }) } doAnswer {
                // new sequence
                sourceChunkIndex.set(100)
                targetChunkIndex.set(100)
                underTest.setSource(source2)
            }
        }
        executeTest()

        inOrder(source, source2, target, states) {
            verify(target).preferableBufferSize
            verify(source).position
            verify(states).invoke(Player.State.Stopped())

            verify(target).start()
            // source
            repeat(chunksFromSource) {
                verify(source).position
                verify(source).getSamples(any())
                if (it == 0) {
                    verify(states).invoke(Player.State.Started())
                }
                verify(target).writeSamples(any())
            }
            verify(source).position
            verify(source).getSamples(any())
            verify(states).invoke(Player.State.Finished((chunksFromSource - 1).toTimestamp()))
            // source2
            repeat(chunksFromSource2) {
                verify(source2).position
                verify(source2).getSamples(any())
                if (it == 0) {
                    verify(states).invoke(Player.State.Started())
                }
                verify(target).writeSamples(any())
            }
            val stopAt = (chunksFromSource2 - 1).toTimestamp()
            verify(source2).position
            verify(source2).getSamples(any())
            verify(states).invoke(Player.State.Finished(stopAt))
            // timeout here
            verify(states).invoke(Player.State.Stopped(stopAt))
            verify(target).stop()
            verify(target).release()
        }
    }

    @Test
    fun `stop and resume playback`() = scope.runTest {
        val chunksBeforeStop = 5
        source.stub {
            on { getSamples(any()) } doAnswer {
                if (0 == (fillNextChunk(it.getArgument(0)) % chunksBeforeStop)) {
                    underTest.stopPlayback()
                }
                true
            }
        }
        executeTest(2)

        inOrder(source, target, states) {
            verify(target).preferableBufferSize
            verify(source).position
            verify(states).invoke(Player.State.Stopped())

            // pass1
            verify(target).start()
            // all chunks but last
            repeat(chunksBeforeStop - 1) {
                verify(source).position
                verify(source).getSamples(any())
                if (it == 0) {
                    verify(states).invoke(Player.State.Started())
                }
                verify(target).writeSamples(any())
            }
            // last chunk is not written
            verify(source).position
            verify(source).getSamples(any())

            verify(states).invoke(Player.State.Stopped((chunksBeforeStop - 2).toTimestamp()))
            verify(target).stop()

            // pass2
            verify(target).start()
            // all chunks but last are written
            repeat(chunksBeforeStop) {
                if (it == 0) {
                    verify(states).invoke(Player.State.Started((chunksBeforeStop - 1).toTimestamp()))
                }
                verify(target).writeSamples(any())
                verify(source).position
                verify(source).getSamples(any())
            }
            verify(states).invoke(Player.State.Stopped((chunksBeforeStop - 2 + chunksBeforeStop).toTimestamp()))
            verify(target).stop()
            verify(target).release()
        }
    }

    @Test
    fun `stop and change source`() = scope.runTest {
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
        states.stub {
            on { states(argThat { this is Player.State.Stopped }) } doReturn Unit doAnswer {
                // new sequence
                sourceChunkIndex.set(100)
                targetChunkIndex.set(100)
                underTest.setSource(source2)
            }
        }

        executeTest(2)

        inOrder(source, source2, target, states) {
            verify(target).preferableBufferSize
            verify(source).position
            verify(states).invoke(Player.State.Stopped())

            // pass1
            verify(target).start()
            // chunks 1&2&3
            repeat(chunksFromSource) {
                verify(source).position
                verify(source).getSamples(any())
                if (it == 0) {
                    verify(states).invoke(Player.State.Started())
                }
                verify(target).writeSamples(any())
            }
            //chunk 4 not written
            verify(source).position
            verify(source).getSamples(any())

            verify(states).invoke(Player.State.Stopped((chunksFromSource - 1).toTimestamp()))
            verify(target).stop()

            // pass2
            verify(target).start()
            repeat(chunksFromSource2) {
                verify(source2).position
                verify(source2).getSamples(any())
                if (it == 0) {
                    verify(states).invoke(Player.State.Started())
                }
                verify(target).writeSamples(any())
            }
            verify(source2).position
            verify(source2).getSamples(any())

            val stopAt = (chunksFromSource2 - 1).toTimestamp()
            verify(states).invoke(Player.State.Finished(stopAt))
            verify(states).invoke(Player.State.Stopped(stopAt))
            verify(target).stop()
            verify(target).release()
        }
    }

    @Test
    fun `set source on the fly`() = scope.runTest {
        val chunksFromSource = 3
        source.stub {
            var doneChunks = 0

            on { getSamples(any()) } doAnswer {
                assert(sourceChunkIndex.get() < 100)
                fillNextChunk(it.getArgument(0))
                if (++doneChunks == chunksFromSource) {
                    underTest.setSource(source2)
                    sourceChunkIndex.set(100)
                }
                true
            }
        }
        var startIndex = 0
        states.stub {
            on { invoke(argThat { this is Player.State.Started }) } doAnswer {
                targetChunkIndex.set(startIndex)
                startIndex += 100
            }
        }

        executeTest()

        inOrder(source, source2, target, states) {
            verify(target).preferableBufferSize
            verify(source).position
            verify(states).invoke(Player.State.Stopped())

            // pass1
            verify(target).start()
            repeat(chunksFromSource) {
                verify(source).position
                verify(source).getSamples(any())
                if (it == 0) {
                    verify(states).invoke(Player.State.Started())
                }
                verify(target).writeSamples(any())
            }

            repeat(chunksFromSource2) {
                verify(source2).position
                verify(source2).getSamples(any())
                if (it == 0) {
                    verify(states).invoke(Player.State.Started())
                }
                verify(target).writeSamples(any())
            }
            verify(source2).position
            verify(source2).getSamples(any())

            val stopAt = (chunksFromSource2 - 1).toTimestamp()
            verify(states).invoke(Player.State.Finished(stopAt))
            verify(states).invoke(Player.State.Stopped(stopAt))
            verify(target).stop()
            verify(target).release()
        }
    }

    @Test
    fun `looped playback events`() = scope.runTest {
        val loopStart = 4
        val loopSize = 3
        val chunksFromSource = 12
        source.stub {
            on { position } doAnswer {
                val idx = sourceChunkIndex.get()
                if (idx <= loopStart) {
                    idx
                } else {
                    loopStart + (idx - loopStart) % loopSize
                }.toTimestamp()
            }

            on { getSamples(any()) } doAnswer {
                fillNextChunk(it.getArgument(0)) <= chunksFromSource
            }
        }

        executeTest()

        inOrder(source, target, states) {
            verify(target).preferableBufferSize
            verify(source).position
            verify(states).invoke(Player.State.Stopped())

            verify(target).start()
            repeat(chunksFromSource) { idx ->
                verify(source).position
                verify(source).getSamples(any())
                when {
                    0 == idx -> verify(states).invoke(Player.State.Started())
                    idx > loopStart && 0 == (idx - loopStart) % loopSize -> verify(states).invoke(
                        Player.State.Started(loopStart.toTimestamp())
                    )
                }
                verify(target).writeSamples(any())
            }
            verify(source).position
            verify(source).getSamples(any())

            val stopAt = (loopStart + (chunksFromSource - loopStart) % loopSize - 1).toTimestamp()
            verify(states).invoke(Player.State.Finished(stopAt))
            verify(states).invoke(Player.State.Stopped(stopAt))
            verify(target).stop()
            verify(target).release()
        }
    }

    @Test
    fun `seek playback events`() = scope.runTest {
        val chunksFromSource = 10
        val seekAt = 7
        val seekTo = 3
        val seekPos = seekTo.toTimestamp()
        source.stub {
            var pos = 0
            on { position } doAnswer { TimeStamp.fromSeconds(pos.toLong()) }
            on { position = any() } doAnswer {
                pos = it.getArgument<TimeStamp>(0).toSeconds().toInt()
            }

            on { getSamples(any()) } doAnswer {
                ++pos
                val done = fillNextChunk(it.getArgument(0))
                if (done == seekAt) {
                    underTest.position = seekPos
                }
                pos <= chunksFromSource
            }
        }

        executeTest()

        inOrder(source, target, states) {
            verify(target).preferableBufferSize
            verify(source).position
            verify(states).invoke(Player.State.Stopped())

            verify(target).start()
            repeat(seekAt) {
                verify(source).position
                verify(source).getSamples(any())
                if (it == 0) {
                    verify(states).invoke(Player.State.Started())
                }
                verify(target).writeSamples(any())
            }
            verify(states).invoke(Player.State.Seeking(seekPos))
            verify(source).position = any()
            repeat(chunksFromSource - seekTo) {
                verify(source).position
                verify(source).getSamples(any())
                if (it == 0) {
                    verify(states).invoke(Player.State.Started(seekPos))
                }
                verify(target).writeSamples(any())
            }
            verify(source).position
            verify(source).getSamples(any())

            val stopAt = (chunksFromSource - 1).toTimestamp()
            verify(states).invoke(Player.State.Finished(stopAt))
            verify(states).invoke(Player.State.Stopped(stopAt))
            verify(target).stop()
            verify(target).release()
        }
    }

    @Test
    fun `seek before start`() {
        val chunksFromSource = 2
        val seekTo = 3
        val seekPos = seekTo.toTimestamp()
        source.stub {
            var pos = 0
            on { position } doAnswer { TimeStamp.fromSeconds(pos.toLong()) }
            on { position = any() } doAnswer {
                pos = it.getArgument<TimeStamp>(0).toSeconds().toInt()
            }

            on { getSamples(any()) } doAnswer {
                ++pos
                fillNextChunk(it.getArgument(0)) <= chunksFromSource
            }
        }
        underTest.use {
            setSource(source)
            assertFalse(isStarted)
            position = seekPos
            startPlayback()
            assertTrue(isStarted)
            while (isStarted) {
                scope.flushEvents()
            }
            assertFalse(isStarted)
        }

        inOrder(source, target, states) {
            verify(target).preferableBufferSize
            verify(source).position
            verify(states).invoke(Player.State.Stopped())

            verify(target).start()
            verify(states).invoke(Player.State.Seeking(seekPos))
            verify(source).position = seekPos
            repeat(chunksFromSource) {
                verify(source).position
                verify(source).getSamples(any())
                if (it == 0) {
                    verify(states).invoke(Player.State.Started(seekPos))
                }
                verify(target).writeSamples(any())
            }
            verify(source).position
            verify(source).getSamples(any())

            val stopAt = (seekTo + chunksFromSource - 1).toTimestamp()
            verify(states).invoke(Player.State.Finished(stopAt))
            verify(states).invoke(Player.State.Stopped(stopAt))
            verify(target).stop()
            verify(target).release()
        }
    }

    private fun executeTest(repeats: Int = 1) = underTest.use {
        assertFalse(isStarted)
        setSource(source)
        repeat(repeats) {
            assertFalse(isStarted)
            startPlayback()
            assertTrue(isStarted)
            while (isStarted) {
                scope.flushEvents()
            }
            assertFalse(isStarted)
        }
    }
}

private fun Int.toTimestamp() = TimeStamp.fromSeconds(toLong())
