package app.zxtune.fs.api

import app.zxtune.BuildConfig
import app.zxtune.net.Http
import org.junit.After
import org.junit.Assert.assertSame
import org.junit.Assert.fail
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.Implementation
import org.robolectric.annotation.Implements
import java.io.IOException
import java.io.InputStream
import java.net.HttpURLConnection
import java.net.URL

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowHttp::class])
class ApiTest {

    private val connection = mock<HttpURLConnection>()
    private var realUrl: String? = null

    @Before
    fun setUp() {
        ShadowHttp.createConnection.stub {
            on { invoke(any()) } doAnswer {
                if (realUrl == null) {
                    realUrl = it.getArgument<String>(0)
                }
                connection
            }
        }
        connection.stub {
            on { responseCode } doReturn HttpURLConnection.HTTP_OK
            on { inputStream } doReturn object : InputStream() {
                override fun read() = -1
            }
            on { url } doAnswer { URL(realUrl) }
        }
        clearInvocations(ShadowHttp.createConnection, connection)
    }

    @After
    fun tearDown() {
        verifyNoMoreInteractions(ShadowHttp.createConnection, connection)
        Api.authorization = null
    }

    @Test
    fun `test workflow`() {
        Api.setAuthorization("Username", "")

        Api.postEvent("event")

        inOrder(ShadowHttp.createConnection, connection).run {
            verify(ShadowHttp.createConnection).invoke("${BuildConfig.API_ROOT}/events/event")
            verify(connection).setRequestProperty("Authorization", "Basic VXNlcm5hbWU6")
            verify(connection).requestMethod = "POST"
            verify(connection).connect()
            verify(connection).responseCode
            verify(connection).inputStream
            verify(connection).url
            verify(connection).disconnect()
        }
    }

    @Test
    fun `test network error`() {
        val error = IOException("Connection error")
        connection.stub {
            on { connect() } doThrow error
        }

        try {
            Api.postEvent("networkerror")
            fail()
        } catch (e: IOException) {
            assertSame(error, e)
        }

        inOrder(ShadowHttp.createConnection, connection).run {
            verify(ShadowHttp.createConnection).invoke("${BuildConfig.API_ROOT}/events/networkerror")
            verify(connection).requestMethod = "POST"
            verify(connection).connect()
            verify(connection).disconnect()
        }
    }

    @Test
    fun `test http error`() {
        connection.stub {
            on { responseCode } doReturn HttpURLConnection.HTTP_NOT_FOUND
        }
        try {
            Api.postEvent("httperror")
            fail()
        } catch (e: IOException) {
        }

        inOrder(ShadowHttp.createConnection, connection).run {
            verify(ShadowHttp.createConnection).invoke("${BuildConfig.API_ROOT}/events/httperror")
            verify(connection).requestMethod = "POST"
            verify(connection).connect()
            verify(connection).responseCode
            verify(connection).responseMessage
            verify(connection).disconnect()
        }
    }

    @Test
    fun `test invalid reply`() {
        connection.stub {
            on { inputStream } doReturn "SomeReply".byteInputStream()
        }
        try {
            Api.postEvent("wrongreply")
            fail()
        } catch (e: IOException) {
        }

        inOrder(ShadowHttp.createConnection, connection).run {
            verify(ShadowHttp.createConnection).invoke("${BuildConfig.API_ROOT}/events/wrongreply")
            verify(connection).requestMethod = "POST"
            verify(connection).connect()
            verify(connection).responseCode
            verify(connection).inputStream
            verify(connection).url
            verify(connection).disconnect()
        }
    }

    @Test
    fun `test unexpected redirect`() {
        realUrl = "https://example.com"
        try {
            Api.postEvent("wrongredirect")
            fail()
        } catch (e: IOException) {
        }

        inOrder(ShadowHttp.createConnection, connection).run {
            verify(ShadowHttp.createConnection).invoke("${BuildConfig.API_ROOT}/events/wrongredirect")
            verify(connection).requestMethod = "POST"
            verify(connection).connect()
            verify(connection).responseCode
            verify(connection).inputStream
            verify(connection).url
            verify(connection).disconnect()
        }
    }
}

@Implements(Http::class)
object ShadowHttp {

    val createConnection = mock<(String) -> HttpURLConnection>()

    @Implementation
    @JvmStatic
    fun createConnection(uri: String) = createConnection.invoke(uri)
}
