package app.zxtune.core.jni

import org.junit.After
import org.junit.Assert.assertSame
import org.junit.Assert.fail
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.mock
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner
import java.nio.ByteBuffer
import java.util.concurrent.CountDownLatch
import java.util.concurrent.atomic.AtomicReference

@RunWith(RobolectricTestRunner::class)
class DelayLoadApiTest {

    private val apiRef = AtomicReference<Api>()
    private val api = mock<Api>()

    @Before
    fun setUp() {
        apiRef.set(null)
    }

    @After
    fun tearDown() {
        verifyNoMoreInteractions(api)
    }

    @Test
    fun `no actions if already loaded`() = mock<Api>().let {
        apiRef.set(it)
        val factory: () -> Api = mock()
        assertSame(it, loadApi(factory))
        verifyNoMoreInteractions(factory)
    }

    @Test
    fun `deferred loading`() {
        assertSame(api, loadApi { api })
    }

    @Test
    fun `blocking methods`() = with(Barrier()) {
        DelayLoadApi(apiRef) { api }.initialize(this)
        apiRef.get().getOptions()
        assertSame(api, apiRef.get())
        verify(api).getOptions()
        waitForRun()
    }

    @Test
    fun `failed to load`() {
        val error = TestException()
        val ref = loadApi { throw error }

        try {
            ref.getOptions()
            unreached()
        } catch (e: RuntimeException) {
            assertSame(error, e.cause)
        }

        try {
            ref.detectModules(ByteBuffer.allocate(1), { _, _ -> unreached() }, null)
            unreached()
        } catch (e: RuntimeException) {
            assertSame(error, e.cause)
        }

        try {
            ref.loadModule(ByteBuffer.allocate(1), "")
            unreached()
        } catch (e: RuntimeException) {
            assertSame(error, e.cause)
        }

        try {
            ref.enumeratePlugins(object : Plugins.Visitor {
                override fun onPlayerPlugin(devices: Int, id: String, description: String) {
                    unreached()
                }

                override fun onContainerPlugin(type: Int, id: String, description: String) {
                    unreached()
                }
            })
            unreached()
        } catch (e: RuntimeException) {
            assertSame(error, e.cause)
        }
    }

    private fun loadApi(factory: () -> Api) = with(Barrier()) {
        DelayLoadApi(apiRef, factory).initialize(this)
        waitForRun()
        apiRef.get()
    }
}

private class Barrier : Runnable {
    private val latch = CountDownLatch(1)

    fun waitForRun() = latch.await()

    override fun run() = latch.countDown()
}

private class TestException : Exception("Failed!!!")

private fun unreached() = fail("Unreached!!!")
