package app.zxtune.fs.provider

import android.content.ContentResolver
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.provider.Settings
import androidx.lifecycle.MutableLiveData
import app.zxtune.Features
import app.zxtune.fs.VfsExtensions
import app.zxtune.fs.VfsObject
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertSame
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(sdk = [Features.StorageAccessFramework.REQUIRED_SDK])
class NotificationsSourceTest {

    private val resolver = mock<ContentResolver>()
    private val context = mock<Context> {
        on { contentResolver } doReturn resolver
    }
    private lateinit var networkState: MutableLiveData<Boolean>
    private lateinit var storageSetupIntent: MutableLiveData<Intent?>
    private lateinit var underTest: NotificationsSource

    private fun getNotification(objUri: Uri) = mock<VfsObject> {
        on { uri } doReturn objUri
    }.let {
        underTest.getNotification(it)
    }

    @Before
    fun setUp() {
        clearInvocations(context, resolver)
        networkState = MutableLiveData()
        storageSetupIntent = MutableLiveData()
        underTest = NotificationsSource(context, networkState, storageSetupIntent)
        assertEquals(true, networkState.hasActiveObservers())
    }

    @After
    fun tearDown() {
        verifyNoMoreInteractions(resolver)
    }

    @Test
    fun `root notifications`() {
        assertEquals(null, getNotification(Uri.EMPTY))
    }

    @Test
    fun `network notification`() {
        val networkNotificationMessage = "Network unavailable"
        context.stub {
            on { getString(any()) } doReturn networkNotificationMessage
        }
        val uri1 = Uri.parse("radio:/")
        val uri2 = Uri.parse("online:/")
        assertEquals(null, getNotification(uri1))
        networkState.value = true
        assertEquals(null, getNotification(uri1))
        networkState.value = false
        getNotification(uri1)!!.run {
            assertEquals(networkNotificationMessage, message)
            assertEquals(Settings.ACTION_WIRELESS_SETTINGS, action!!.action)
        }
        getNotification(uri2)!!.run {
            assertEquals(networkNotificationMessage, message)
            assertEquals(Settings.ACTION_WIRELESS_SETTINGS, action!!.action)
        }
        networkState.value = true
        assertEquals(null, getNotification(uri2))

        inOrder(resolver) {
            verify(resolver, times(2)).notifyChange(Query.notificationUriFor(uri1), null)
            verify(resolver).notifyChange(Query.notificationUriFor(uri2), null)
        }
    }

    @Test
    fun `storage notification`() {
        val storageNotificationMessage = "No access. Tap to fix"
        context.stub {
            on { getString(any()) } doReturn storageNotificationMessage
        }
        val uri1 = Uri.parse("file:")
        val uri2 = Uri.parse("file://authority")
        val uri2PermissionIntent = Intent("request", Uri.parse("permission://authority"))
        val uri3 = Uri.parse("file://authority/path")

        var objUri = uri1
        var permissionIntent: Intent? = null
        val dir = mock<VfsObject> {
            on { uri }.doAnswer {
                objUri
            }
            on { getExtension(VfsExtensions.PERMISSION_QUERY_INTENT) } doAnswer {
                permissionIntent
            }
        }
        assertEquals(null, underTest.getNotification(dir))
        objUri = uri2
        permissionIntent = uri2PermissionIntent
        underTest.getNotification(dir)!!.run {
            assertEquals(storageNotificationMessage, message)
            assertSame(uri2PermissionIntent, action)
        }
        objUri = uri3
        permissionIntent = null
        assertEquals(null, underTest.getNotification(dir))
    }

    @Test
    fun `playlist notifications`() {
        val playlistNotificationMessage = "No persistent storage set up"
        context.stub {
            on { getString(any()) } doReturn playlistNotificationMessage
        }
        val uri1 = Uri.parse("playlists:")
        val uri2 = Uri.parse("playlists://somefile")
        var objUri = uri1
        val dir = mock<VfsObject> {
            on { uri }.doAnswer {
                objUri
            }
        }
        val defaultLocation = Uri.parse("scheme://host/path")
        storageSetupIntent.value = Intent("action", defaultLocation)
        underTest.getNotification(dir)!!.run {
            assertEquals(playlistNotificationMessage, message)
            assertEquals(defaultLocation, action?.data)
        }
        storageSetupIntent.value = null
        assertEquals(null, underTest.getNotification(dir))
        objUri = uri2
        assertEquals(null, underTest.getNotification(dir))

        verify(resolver).notifyChange(Query.notificationUriFor(uri1), null)
    }
}
