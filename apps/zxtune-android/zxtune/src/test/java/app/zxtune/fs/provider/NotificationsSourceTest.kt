package app.zxtune.fs.provider

import android.content.ContentResolver
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.provider.Settings
import app.zxtune.Features
import app.zxtune.TestUtils.flushEvents
import app.zxtune.fs.VfsExtensions
import app.zxtune.fs.VfsObject
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertSame
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.clearInvocations
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.stub
import org.mockito.kotlin.times
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config

@RunWith(RobolectricTestRunner::class)
@Config(sdk = [Features.StorageAccessFramework.REQUIRED_SDK])
class NotificationsSourceTest {

    private val dispatcher = StandardTestDispatcher()

    private val resolver = mock<ContentResolver>()
    private val context = mock<Context> {
        on { contentResolver } doReturn resolver
    }
    private lateinit var networkState: MutableSharedFlow<Boolean>
    private lateinit var storageSetupIntent: MutableSharedFlow<Intent?>
    private lateinit var underTest: NotificationsSource

    private fun getNotification(objUri: Uri) = mock<VfsObject> {
        on { uri } doReturn objUri
    }.let {
        underTest.getNotification(it)
    }

    @Before
    fun setUp() {
        clearInvocations(context, resolver)
        networkState = MutableSharedFlow(replay = 1)
        storageSetupIntent = MutableSharedFlow(replay = 1)
        underTest = NotificationsSource(context, networkState, storageSetupIntent, dispatcher)
    }

    @After
    fun tearDown() {
        underTest.shutdown()
        verifyNoMoreInteractions(resolver)
    }

    @Test
    fun `root notifications`() {
        assertEquals(null, getNotification(Uri.EMPTY))
    }

    @Test
    fun `network notification`() = runTest(dispatcher) {
        val networkNotificationMessage = "Network unavailable"
        context.stub {
            on { getString(any()) } doReturn networkNotificationMessage
        }
        val uri1 = Uri.parse("radio:/")
        val uri2 = Uri.parse("online:/")
        assertEquals(null, getNotification(uri1))
        networkState.emit(true)
        flushEvents()
        assertEquals(null, getNotification(uri1))

        networkState.emit(false)
        flushEvents()
        requireNotNull(getNotification(uri1)).run {
            assertEquals(networkNotificationMessage, message)
            assertEquals(Settings.ACTION_WIRELESS_SETTINGS, action!!.action)
        }
        requireNotNull(getNotification(uri2)).run {
            assertEquals(networkNotificationMessage, message)
            assertEquals(Settings.ACTION_WIRELESS_SETTINGS, action!!.action)
        }

        networkState.emit(true)
        flushEvents()
        assertEquals(null, getNotification(uri2))

        inOrder(resolver) {
            verify(resolver, times(3)).notifyChange(Query.notificationUriFor(Uri.EMPTY), null)
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
    fun `playlist notifications`() = runTest(dispatcher) {
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
        storageSetupIntent.emit(Intent("action", defaultLocation))
        flushEvents()
        requireNotNull(underTest.getNotification(dir)).run {
            assertEquals(playlistNotificationMessage, message)
            assertEquals(defaultLocation, action?.data)
        }
        storageSetupIntent.emit(null)
        flushEvents()
        assertEquals(null, underTest.getNotification(dir))
        objUri = uri2
        assertEquals(null, underTest.getNotification(dir))

        // value, null
        verify(resolver, times(2)).notifyChange(Query.notificationUriFor(Uri.EMPTY), null)
    }
}
