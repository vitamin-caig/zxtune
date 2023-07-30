package app.zxtune.device.media

import android.content.ComponentName
import android.content.Context
import android.support.v4.media.MediaBrowserCompat
import androidx.lifecycle.Observer
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class MediaBrowserConnectionTest {
    private val ctx = mock<Context> {
        on { packageName } doReturn "SomePackage"
    }
    private val browser = mock<MediaBrowserCompat>()
    private val factory = mock<MakeBrowser>()

    @After
    fun tearDown() {
        verifyNoMoreInteractions(browser)
    }

    @Test
    fun `factory arguments`() {
        factory.stub {
            on { invoke(any(), any(), any(), anyOrNull()) } doReturn browser
        }
        val underTest = MediaBrowserConnection(ctx, factory)
        verify(factory).invoke(
            any(),
            eq(ComponentName("SomePackage", "app.zxtune.MainService")),
            any(),
            eq(null)
        )
        verifyNoMoreInteractions(factory)
    }

    @Test
    fun `ondemand connection`() {
        lateinit var callback: MediaBrowserCompat.ConnectionCallback
        factory.stub {
            on { invoke(any(), any(), any(), anyOrNull()) } doAnswer {
                callback = it.getArgument(2)
                browser
            }
        }
        browser.stub {
            on { connect() } doAnswer {
                callback.onConnected()
            }
            on { disconnect() } doAnswer {
                callback.onConnectionFailed()
            }
        }
        with(MediaBrowserConnection(ctx, factory)) {
            assertEquals(null, value)
            val observer = mock<Observer<MediaBrowserCompat?>>()
            observer.let {
                observeForever(it)
                assertEquals(browser, value)
                removeObserver(it)
            }
            assertEquals(null, value)
            observer.let {
                observeForever(it)
                assertEquals(browser, value)
                removeObserver(it)
            }
            assertEquals(null, value)
        }
        inOrder(browser) {
            verify(browser).connect()
            verify(browser).disconnect()
            verify(browser).connect()
            verify(browser).disconnect()
        }
    }

    @Test
    fun `reconnect while long connection`() {
        lateinit var callback: MediaBrowserCompat.ConnectionCallback
        factory.stub {
            on { invoke(any(), any(), any(), anyOrNull()) } doAnswer {
                callback = it.getArgument(2)
                browser
            }
        }
        with(MediaBrowserConnection(ctx, factory)) {
            assertEquals(null, value)
            val observer = mock<Observer<MediaBrowserCompat?>>()
            // no callback calls here
            observer.let {
                observeForever(it)
                assertEquals(null, value)
                removeObserver(it)
            }
            assertEquals(null, value)
            callback.onConnected()
            assertEquals(browser, value)
            observer.let {
                observeForever(it)
                assertEquals(browser, value)
                removeObserver(it)
                callback.onConnectionSuspended()
            }
            assertEquals(null, value)
        }
        inOrder(browser) {
            verify(browser).connect()
            verify(browser).disconnect()
        }
    }
}
