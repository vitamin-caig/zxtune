package app.zxtune.fs.provider

import android.content.ContentProvider
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.CancellationSignal
import android.os.OperationCanceledException
import android.provider.Settings
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import app.zxtune.Features
import app.zxtune.fs.TestDir
import app.zxtune.fs.TestFile
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsExtensions
import app.zxtune.fs.VfsFile
import app.zxtune.fs.VfsObject
import app.zxtune.net.NetworkManager
import app.zxtune.use
import app.zxtune.utils.AsyncWorker
import app.zxtune.utils.ProgressCallback
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.argThat
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.doThrow
import org.mockito.kotlin.eq
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.reset
import org.mockito.kotlin.stub
import org.mockito.kotlin.times
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.Robolectric
import org.robolectric.RobolectricTestRunner
import org.robolectric.android.controller.ContentProviderController
import org.robolectric.annotation.Config
import org.robolectric.annotation.Implementation
import org.robolectric.annotation.Implements
import java.io.IOException

// tests both Provider and VfsClient
@RunWith(RobolectricTestRunner::class)
@Config(
    shadows = [ShadowNetworkManager::class], sdk = [Features.StorageAccessFramework.REQUIRED_SDK]
)
class ClientProviderTest {

    private val fastDirContent = Array(10) { TestDir(2 + it) }
    private val fastDir = object : TestDir(1) {
        override fun enumerate(visitor: VfsDir.Visitor) = fastDirContent.forEach(visitor::onDir)

        override val parent: VfsObject? = null
    }
    private val fastUri = fastDir.uri

    private val slowDirContent = Array(5) { TestFile(11 + it, "Unused") }
    private val slowDir = object : TestDir(10) {
        override fun enumerate(visitor: VfsDir.Visitor) {
            slowDirContent.forEachIndexed { index, obj ->
                kotlin.runCatching { Thread.sleep(1200) }
                visitor.onProgressUpdate(index + 1, slowDirContent.size)
                visitor.onFile(obj)
            }
        }

        override val parent = fastDir
    }
    private val slowUri = slowDir.uri

    private val deepDir = object : TestDir(100) {
        override fun enumerate(visitor: VfsDir.Visitor) = Unit
        override val parent = slowDir
    }
    private val deepUri = deepDir.uri

    private val failedDir = object : TestDir(1000) {
        override fun enumerate(visitor: VfsDir.Visitor) {
            throw IOException("Failed to enumerate")
        }
    }
    private val failedUri = failedDir.uri

    private val hangingDir = object : TestDir(10000) {
        override fun enumerate(visitor: VfsDir.Visitor) {
            Thread.sleep(10000000L)
        }
    }
    private val hangingUri = hangingDir.uri

    private val unknownUri = fastDirContent.first().uri

    private val resolver = mock<Resolver> {
        on { resolve(fastUri) } doReturn fastDir
        on { resolve(slowUri) } doReturn slowDir
        on { resolve(deepUri) } doReturn deepDir
        on { resolve(failedUri) } doReturn failedDir
        on { resolve(hangingUri) } doReturn hangingDir
        on { resolve(eq(fastUri), any()) } doReturn fastDir
        on { resolve(eq(slowUri), any()) } doAnswer {
            val progress = it.getArgument<ProgressCallback>(1)
            for (idx in 10..50 step 10) {
                Thread.sleep(1100)
                progress.onProgressUpdate(idx, 50)
            }
            slowDir
        }
        on { resolve(eq(failedUri), any()) } doThrow Error("Failed to resolve")
    }
    private val schema = mock<SchemaSource> {
        on { resolved(any()) } doAnswer {
            when (val arg = it.getArgument<VfsObject>(0)) {
                is VfsDir -> convert(arg)
                is VfsFile -> convert(arg)
                else -> null
            }
        }
        on { parents(any()) } doAnswer {
            it.getArgument<List<VfsObject>>(0).map(::convertParent)
        }
        on { directories(any()) } doAnswer {
            it.getArgument<List<VfsDir>>(0).map { dir -> mock.resolved(dir) as Schema.Listing.Dir }
        }
        on { files(any()) } doAnswer {
            it.getArgument<List<VfsFile>>(0)
                .map { file -> mock.resolved(file) as Schema.Listing.File }
        }
    }

    private lateinit var provider: ContentProvider
    private lateinit var client: VfsProviderClient
    private val listingCallback = mock<VfsProviderClient.ListingCallback>()
    private val parentsCallback = mock<VfsProviderClient.ParentsCallback>()

    private fun convert(arg: VfsDir) =
        Schema.Listing.Dir(arg.uri, arg.name, arg.description, null, false)

    private fun convert(arg: VfsFile) =
        Schema.Listing.File(arg.uri, arg.name, arg.description, arg.size, null, null)

    private fun convertParent(obj: VfsObject) = Schema.Parents.Object(obj.uri, obj.name, null)

    @Before
    fun setUp() {
        provider = ContentProviderController.of(Provider(resolver, schema)).create().get()
        client = VfsProviderClient(provider.context!!)
        reset(listingCallback, parentsCallback)
    }

    @After
    fun tearDown() {
        provider.shutdown()
        verifyNoMoreInteractions(listingCallback, parentsCallback)
    }

    @Test
    fun `resolve unknown`() {
        client.resolve(unknownUri, listingCallback)
    }

    @Test
    fun `resolve failed`() {
        val ex = assertThrows<Exception> {
            client.resolve(failedUri, listingCallback)
        }
        assertEquals("Failed to resolve", ex.message)
    }

    @Test
    fun `resolve fast`() {
        client.resolve(fastUri, listingCallback)
        verify(listingCallback).onDir(convert(fastDir))
    }

    @Test
    fun `resolve slow`() {
        client.resolve(slowUri, listingCallback)
        inOrder(listingCallback) {
            // dump progress first
            verify(listingCallback, times(5)).onProgress(argThat { total == 50 })
            verify(listingCallback).onDir(convert(slowDir))
        }
    }

    @Test
    fun `list unknown`() {
        client.list(unknownUri, listingCallback)
    }

    @Test
    fun `list failed`() {
        val ex = assertThrows<Exception> {
            client.list(failedUri, listingCallback)
        }
        assertEquals("Failed to enumerate", ex.message)
    }

    @Test
    fun `list fast`() {
        client.list(fastUri, listingCallback)
        fastDirContent.forEach {
            verify(listingCallback).onDir(convert(it))
        }
    }

    @Test
    fun `list slow`() {
        client.list(slowUri, listingCallback)
        val elements = slowDirContent.size
        inOrder(listingCallback) {
            // dump progress first
            for (done in 1..elements) {
                verify(listingCallback).onProgress(Schema.Status.Progress(done, elements))
            }
            slowDirContent.forEach {
                verify(listingCallback).onFile(convert(it))
            }
        }
    }

    @Test
    fun `list empty`() {
        client.list(deepUri, listingCallback)
    }

    @Test
    fun `parents chain`() {
        client.parents(deepUri, parentsCallback)
        inOrder(parentsCallback) {
            verify(parentsCallback).onObject(convertParent(fastDir))
            verify(parentsCallback).onObject(convertParent(slowDir))
            verify(parentsCallback).onObject(convertParent(deepDir))
        }
    }

    @Test
    fun `parents empty`() {
        client.parents(fastUri, parentsCallback)
        verify(parentsCallback).onObject(convertParent(fastDir))
    }

    @Test
    fun `search slow`() {
        client.search(slowUri, "object", listingCallback)
        slowDirContent.forEach {
            verify(listingCallback).onFile(convert(it))
        }
    }

    @Test
    fun `search empty`() {
        client.search(deepUri, "object", listingCallback)
    }

    @Test
    fun `network state notification`() {
        val networkUri = Uri.parse("radio:/")
        resolver.stub {
            on { resolve(networkUri) } doAnswer {
                mock<VfsDir> {
                    on { uri } doReturn networkUri
                }
            }
        }
        val notifications = ArrayList<Schema.Notifications.Object?>()
        // First notification is delivered immediately
        client.subscribeForNotifications(networkUri, notifications::add).use {
            ShadowNetworkManager.state.value = false
            while (notifications.size != 2) {
                Robolectric.flushForegroundThreadScheduler()
            }
            ShadowNetworkManager.state.value = true
            while (notifications.size != 3) {
                Robolectric.flushForegroundThreadScheduler()
            }
        }
        assertEquals(3, notifications.size)
        assertEquals(null, notifications[0])
        notifications[1]!!.run {
            assertEquals("Network is not accessible", message)
            assertEquals(Settings.ACTION_WIRELESS_SETTINGS, action!!.action)
        }
        assertEquals(null, notifications[2])
    }

    @Test
    fun `storage notification`() {
        val noPermissionsUri = Uri.parse("file://root/path/to/dir")
        val noPermissionsIntent = Intent("action", noPermissionsUri)
        var permissionQueryIntent: Intent? = null
        val noPermissionsDir = object : TestDir(20000) {
            override val uri: Uri
                get() = noPermissionsUri

            override fun getExtension(id: String) = when (id) {
                VfsExtensions.PERMISSION_QUERY_INTENT -> permissionQueryIntent
                else -> super.getExtension(id)
            }
        }
        resolver.stub {
            on { resolve(noPermissionsUri) } doReturn noPermissionsDir
        }

        permissionQueryIntent = noPermissionsIntent
        client.subscribeForNotifications(noPermissionsUri) { notification ->
            assertEquals("Tap to give access permission", notification!!.message)
            notification.action!!.run {
                assertEquals(noPermissionsIntent.action, action)
                assertEquals(noPermissionsIntent.data, data)
            }
        }.release()
        permissionQueryIntent = null
        client.subscribeForNotifications(noPermissionsUri) { notification ->
            assertEquals(null, notification)
        }.release()
    }

    @Test
    fun `client exception`() {
        listingCallback.stub {
            on { onProgress(any()) } doThrow Error("Client cancellation")
        }
        val ex = assertThrows<Exception> {
            client.list(slowUri, listingCallback)
        }
        assertEquals(OperationCanceledException().message, ex.message)
        verify(listingCallback).onProgress(any())
    }

    @Test
    fun `client cancellation`() {
        val signal = CancellationSignal()
        listingCallback.stub {
            on { onProgress(any()) } doAnswer {
                signal.cancel()
            }
        }
        val ex = assertThrows<Exception> {
            client.list(slowUri, listingCallback, signal)
        }
        assertEquals(OperationCanceledException().message, ex.message)
        verify(listingCallback).onProgress(any())
    }

    @Test
    fun `client interruption`() {
        val signal = CancellationSignal()
        AsyncWorker("unused").execute {
            Thread.sleep(1000)
            signal.cancel()
        }
        val ex = assertThrows<Exception> {
            client.list(hangingUri, listingCallback, signal)
        }
        assertEquals("sleep interrupted", ex.message)
    }
}

@Implements(NetworkManager::class)
class ShadowNetworkManager {

    companion object {
        val state = MutableLiveData<Boolean>()

        @JvmStatic
        @Implementation
        fun initialize(ctx: Context) = Unit

        @JvmStatic
        @get:Implementation
        val networkAvailable: LiveData<Boolean>
            get() = state
    }
}
