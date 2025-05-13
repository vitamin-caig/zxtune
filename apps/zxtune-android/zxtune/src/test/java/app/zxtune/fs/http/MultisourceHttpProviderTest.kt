package app.zxtune.fs.http

import android.net.Uri
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.InOrder
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import java.io.IOException
import java.io.InputStream

@RunWith(RobolectricTestRunner::class)
class MultisourceHttpProviderTest {

    companion object {
        private val goodHost = "good.host"
        private val badHost = "bad.host"
        private val goodHostUri = Uri.Builder().scheme("http").authority(goodHost).build()
        private val badHostUri = Uri.Builder().scheme("http").authority(badHost).build()
        private val stream = mock<InputStream>()
        private val httpObject = mock<HttpObject>()
        private val err = IOException("Error!")

        private fun urisWithLimiterOf(vararg uris: Uri) = iterator {
            assertEquals(goodHostUri, uris.last())
            for (uri in uris) {
                yield(uri)
            }
            fail("Should not be called")
        }
    }

    private val delegate = mock<HttpProvider> {
        on { getInputStream(goodHostUri) } doReturn stream
        on { getInputStream(badHostUri) } doThrow err
        on { getObject(goodHostUri) } doReturn httpObject
        on { getObject(badHostUri) } doThrow err
        on { hasConnection() } doReturn true
    }
    private val timeSource = mock<MultisourceHttpProvider.TimeSource>()

    lateinit var underTest: MultisourceHttpProvider

    @Before
    fun setUp() {
        underTest = MultisourceHttpProvider(delegate, timeSource)
    }

    @After
    fun tearDown() {
        verifyNoMoreInteractions(delegate, timeSource)
    }

    @Test
    fun `test getInputStream single url`() {
        assertSame(stream, underTest.getInputStream(goodHostUri))
        verify(delegate).getInputStream(goodHostUri)
    }

    @Test
    fun `test getInputStream single url error`() {
        try {
            underTest.getInputStream(badHostUri)
            fail()
        } catch (e: IOException) {
            assertSame(e, err)
        }
        verify(delegate).getInputStream(badHostUri)
    }

    @Test
    fun `test successful url`() {
        assertSame(stream, underTest.getInputStream(urisWithLimiterOf(goodHostUri)))
        assertSame(httpObject, underTest.getObject(urisWithLimiterOf(goodHostUri)))

        inOrder(delegate, timeSource).run {
            verify(timeSource).nowMillis()
            verify(delegate).getInputStream(goodHostUri)
            verify(timeSource).nowMillis()
            verify(delegate).getObject(goodHostUri)
        }
    }

    @Test
    fun `test getInputStream one url error`() {
        try {
            underTest.getInputStream(arrayOf(badHostUri).iterator())
            fail()
        } catch (e: IOException) {
            assertSame(e, err)
        }
        inOrder(delegate, timeSource).run {
            verify(timeSource).nowMillis()
            verify(delegate).getInputStream(badHostUri)
            verify(delegate).hasConnection()
        }
    }

    @Test
    fun `test backoff`() {
        var now = 100L
        doAnswer { now }.whenever(timeSource).nowMillis()
        // no ban for the first error
        run {
            assertSame(stream, underTest.getInputStream(urisWithLimiterOf(badHostUri, goodHostUri)))
            assertFalse(underTest.isHostDisabledFor(badHost, now + 1))

            inOrderVerifyAndReset(delegate, timeSource) {
                verify(timeSource).nowMillis()
                verify(delegate).getInputStream(badHostUri)
                verify(delegate).hasConnection()
                verify(delegate).getInputStream(goodHostUri)
            }
        }

        // ban for 1s
        run {
            now += 100
            assertSame(httpObject, underTest.getObject(urisWithLimiterOf(badHostUri, goodHostUri)))
            verifyBadHostBannedTill(now + 1000)

            inOrderVerifyAndReset(delegate, timeSource) {
                verify(timeSource).nowMillis()
                verify(delegate).getObject(badHostUri)
                verify(delegate).hasConnection()
                verify(delegate).getObject(goodHostUri)
            }
        }
        // no request to bad host
        run {
            now += 100
            assertSame(stream, underTest.getInputStream(urisWithLimiterOf(badHostUri, goodHostUri)))

            inOrderVerifyAndReset(delegate, timeSource) {
                verify(timeSource).nowMillis()
                verify(delegate).getInputStream(goodHostUri)
            }
        }
        // next ban for 2s
        run {
            now += 1000
            assertSame(httpObject, underTest.getObject(urisWithLimiterOf(badHostUri, goodHostUri)))
            verifyBadHostBannedTill(now + 2000)

            inOrderVerifyAndReset(delegate, timeSource) {
                verify(timeSource).nowMillis()
                verify(delegate).getObject(badHostUri)
                verify(delegate).hasConnection()
                verify(delegate).getObject(goodHostUri)
            }
        }
        // linear bans with maximal of 1h
        run {
            for (delay in 2..3600) {
                now += delay * 1000
                verifyBadHostBannedTill(now)
                assertSame(stream, underTest.getInputStream(urisWithLimiterOf(badHostUri, goodHostUri)))
            }
            clearInvocations(delegate, timeSource)

            now += 10_000_000
            assertSame(httpObject, underTest.getObject(urisWithLimiterOf(badHostUri, goodHostUri)))
            verifyBadHostBannedTill(now + 3600_000)

            inOrderVerifyAndReset(delegate, timeSource) {
                verify(timeSource).nowMillis()
                verify(delegate).getObject(badHostUri)
                verify(delegate).hasConnection()
                verify(delegate).getObject(goodHostUri)
            }
        }
        // restored
        run {
            doAnswer { stream }.whenever(delegate).getInputStream(badHostUri)
            now += 3600_000
            assertSame(stream, underTest.getInputStream(urisWithLimiterOf(badHostUri, goodHostUri)))

            inOrderVerifyAndReset(delegate, timeSource) {
                verify(timeSource).nowMillis()
                verify(delegate).getInputStream(badHostUri)
            }
        }
        // backoff depends on failures/successes ratio
        run {
            repeat(1800) {
                now += 1000
                assertSame(stream, underTest.getInputStream(urisWithLimiterOf(badHostUri, goodHostUri)))
            }
            clearInvocations(delegate, timeSource)

            now += 1000
            assertSame(httpObject, underTest.getObject(urisWithLimiterOf(badHostUri, goodHostUri)))
            inOrderVerifyAndReset(delegate, timeSource) {
                verify(timeSource).nowMillis()
                verify(delegate).getObject(badHostUri)
                verify(delegate).hasConnection()
                verify(delegate).getObject(goodHostUri)
            }
            // first error after success is ignored
            assertFalse(underTest.isHostDisabledFor(badHost, now))

            now += 1000
            assertSame(httpObject, underTest.getObject(urisWithLimiterOf(badHostUri, goodHostUri)))
            // 3605 failed and 1801 successful requests
            verifyBadHostBannedTill(now + 1803_000)
            inOrderVerifyAndReset(delegate, timeSource) {
                verify(timeSource).nowMillis()
                verify(delegate).getObject(badHostUri)
                verify(delegate).hasConnection()
                verify(delegate).getObject(goodHostUri)
            }
        }
        // network errors are not taken into account
        run {
            now += 2000_000

            delegate.stub { on { hasConnection() } doReturn false }
            try {
                underTest.getObject(urisWithLimiterOf(badHostUri, goodHostUri))
                fail()
            } catch (e: IOException) {
                assertSame(e, err)
            }
            assertFalse(underTest.isHostDisabledFor(badHost, now))
            inOrderVerifyAndReset(delegate, timeSource) {
                verify(timeSource).nowMillis()
                verify(delegate).getObject(badHostUri)
                verify(delegate).hasConnection()
            }
        }
    }

    private fun verifyBadHostBannedTill(time: Long) {
        assertTrue(underTest.isHostDisabledFor(badHost, time - 1))
        assertFalse(underTest.isHostDisabledFor(badHost, time))
    }
}

private fun inOrderVerifyAndReset(vararg mocks: Any, cmd: InOrder.() -> Unit) {
    cmd(inOrder(*mocks))
    verifyNoMoreInteractions(*mocks)
    clearInvocations(*mocks)
}
