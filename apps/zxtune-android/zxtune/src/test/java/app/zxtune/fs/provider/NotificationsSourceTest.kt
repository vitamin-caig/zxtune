package app.zxtune.fs.provider

import android.content.ContentResolver
import android.content.Context
import android.net.Uri
import android.provider.Settings
import androidx.lifecycle.MutableLiveData
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class NotificationsSourceTest {

    private val resolver = mock<ContentResolver>()
    private val context = mock<Context> {
        on { contentResolver } doReturn resolver
    }
    private lateinit var networkState: MutableLiveData<Boolean>
    private lateinit var underTest: NotificationsSource

    @Before
    fun setUp() {
        clearInvocations(context, resolver)
        networkState = MutableLiveData()
        underTest = NotificationsSource(context, networkState)
        assertEquals(true, networkState.hasActiveObservers())
    }

    @After
    fun tearDown() {
        verifyNoMoreInteractions(resolver)
    }

    @Test
    fun `root notifications`() {
        assertEquals(null, underTest.getNotification(Uri.EMPTY))
    }

    @Test
    fun `network notification`() {
        val networkNotificationMessage = "Network unavailable"
        context.stub {
            on { getString(any()) } doReturn networkNotificationMessage
        }
        val uri1 = Uri.parse("radio:/")
        val uri2 = Uri.parse("online:/")
        assertEquals(null, underTest.getNotification(uri1))
        networkState.value = true
        assertEquals(null, underTest.getNotification(uri1))
        networkState.value = false
        underTest.getNotification(uri1)!!.run {
            assertEquals(networkNotificationMessage, message)
            assertEquals(Settings.ACTION_WIRELESS_SETTINGS, action!!.action)
        }
        underTest.getNotification(uri2)!!.run {
            assertEquals(networkNotificationMessage, message)
            assertEquals(Settings.ACTION_WIRELESS_SETTINGS, action!!.action)
        }
        networkState.value = true
        assertEquals(null, underTest.getNotification(uri2))

        inOrder(resolver) {
            verify(resolver, times(2)).notifyChange(Query.notificationUriFor(uri1), null)
            verify(resolver).notifyChange(Query.notificationUriFor(uri2), null)
        }
    }
}