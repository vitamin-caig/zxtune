package app.zxtune.core.jni

import app.zxtune.assertThrows
import app.zxtune.core.Module
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertSame
import org.junit.Assert.fail
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.doThrow
import org.mockito.kotlin.mock
import org.mockito.kotlin.stub
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner
import java.nio.ByteBuffer

@RunWith(RobolectricTestRunner::class)
class DelayLoadApiTest {

    private val api = mock<Api>()

    @Before
    fun setUp() {
        JniApi.create = mock {
            on { invoke() } doReturn api
        }
        Api.loaderCache.set(null)
    }

    @After
    fun tearDown() = verifyNoMoreInteractions(JniApi.create, api)

    @Test
    fun `deferred loading`() = runTest {
        val res = Api.load().await()
        assertSame(api, res)
        verify(JniApi.create).invoke()
    }

    @Test
    fun `blocking methods`() {
        val res = Api.instance()
        assertSame(api, res)
        verify(JniApi.create).invoke()
    }

    @Test
    fun `error stub`() = runTest {
        val error = TestException()
        JniApi.create.stub {
            on { invoke() } doThrow error
        }
        val res = Api.load().await()
        assertThrows<RuntimeException> {
            res.getOptions()
        }.let { e ->
            assertSame(error, e.cause)
        }

        assertThrows<RuntimeException> {
            res.detectModules(ByteBuffer.allocate(1), object : DetectCallback {
                override fun onModule(subPath: String, obj: Module) = unreached()
                override fun onPicture(subPath: String, obj: ByteArray) = unreached()
            }, null)
        }.let { e ->
            assertSame(error, e.cause)
        }

        assertThrows<RuntimeException> {
            res.loadModule(ByteBuffer.allocate(1), "")
        }.let { e ->
            assertSame(error, e.cause)
        }

        assertThrows<RuntimeException> {
            res.enumeratePlugins(object : Plugins.Visitor {
                override fun onPlayerPlugin(devices: Int, id: String, description: String) =
                    unreached()

                override fun onContainerPlugin(type: Int, id: String, description: String) =
                    unreached()
            })
        }.let { e ->
            assertSame(error, e.cause)
        }
        verify(JniApi.create).invoke()
    }
}

private class TestException : RuntimeException("Failed!!!")

private fun unreached() = fail("Unreached!!!")
