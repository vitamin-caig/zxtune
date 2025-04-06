package app.zxtune.fs.provider

import android.content.ContentProvider
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.provider.Settings
import app.zxtune.Features
import app.zxtune.TestUtils.flushEvents
import app.zxtune.TestUtils.mockCollectorOf
import app.zxtune.assertThrows
import app.zxtune.fs.TestDir
import app.zxtune.fs.TestFile
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsExtensions
import app.zxtune.fs.VfsFile
import app.zxtune.fs.VfsObject
import app.zxtune.net.NetworkManager
import app.zxtune.ui.MainDispatcherRule
import app.zxtune.utils.ProgressCallback
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Rule
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

    private val dispatcher = StandardTestDispatcher()

    // to inject to NotificationSource
    @get:Rule
    val mainDispatcher = MainDispatcherRule(dispatcher)

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
                Thread.sleep(1200)
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
    private lateinit var underTest: VfsProviderClient
    private val listingCallback = mock<VfsProviderClient.ListingCallback>()
    private val parentsCallback = mock<VfsProviderClient.ParentsCallback>()

    private fun convert(arg: VfsDir) =
        Schema.Listing.Dir(arg.uri, arg.name, arg.description, null, false)

    private fun convert(arg: VfsFile) =
        Schema.Listing.File(arg.uri, arg.name, arg.description, arg.size, Schema.Listing.File.Type.UNKNOWN)

    private fun convertParent(obj: VfsObject) = Schema.Parents.Object(obj.uri, obj.name, null)

    @Before
    fun setUp() {
        provider = ContentProviderController.of(Provider(resolver, schema)).create().get()
        underTest = VfsProviderClient(provider.context!!)
        reset(listingCallback, parentsCallback)
    }

    @After
    fun tearDown() {
        provider.shutdown()
        verifyNoMoreInteractions(listingCallback, parentsCallback)
    }

    @Test
    fun `resolve unknown`() = runTest {
        underTest.resolve(unknownUri, listingCallback)
    }

    @Test
    fun `resolve failed`() = runTest {
        val ex = assertThrows<Exception> {
            underTest.resolve(failedUri, listingCallback)
        }
        assertEquals("Failed to resolve", ex.message)
    }

    @Test
    fun `resolve fast`() = runTest {
        underTest.resolve(fastUri, listingCallback)
        verify(listingCallback).onDir(convert(fastDir))
    }

    @Test
    fun `resolve slow`() = runTest {
        underTest.resolve(slowUri, listingCallback)
        inOrder(listingCallback) {
            // dump progress first
            verify(listingCallback, times(5)).onProgress(argThat { total == 50 })
            verify(listingCallback).onDir(convert(slowDir))
        }
    }

    @Test
    fun `list unknown`() = runTest {
        underTest.list(unknownUri, listingCallback)
    }

    @Test
    fun `list failed`() = runTest {
        val ex = assertThrows<Exception> {
            underTest.list(failedUri, listingCallback)
        }
        assertEquals("Failed to enumerate", ex.message)
    }

    @Test
    fun `list fast`() = runTest {
        underTest.list(fastUri, listingCallback)
        fastDirContent.forEach {
            verify(listingCallback).onDir(convert(it))
        }
    }

    @Test
    fun `list slow`() = runTest {
        underTest.list(slowUri, listingCallback)
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
    fun `list empty`() = runTest {
        underTest.list(deepUri, listingCallback)
    }

    @Test
    fun `parents chain`() = runTest {
        underTest.parents(deepUri, parentsCallback)
        inOrder(parentsCallback) {
            verify(parentsCallback).onObject(convertParent(fastDir))
            verify(parentsCallback).onObject(convertParent(slowDir))
            verify(parentsCallback).onObject(convertParent(deepDir))
        }
    }

    @Test
    fun `parents empty`() = runTest {
        underTest.parents(fastUri, parentsCallback)
        verify(parentsCallback).onObject(convertParent(fastDir))
    }

    @Test
    fun `search slow`() = runTest {
        underTest.search(slowUri, "object", listingCallback)
        slowDirContent.forEach {
            verify(listingCallback).onFile(convert(it))
        }
    }

    @Test
    fun `search empty`() = runTest {
        underTest.search(deepUri, "object", listingCallback)
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
        runTest(dispatcher) {
            val mockNotifications = mockCollectorOf(underTest.observeNotifications(networkUri))
            inOrder(mockNotifications) {
                verify(mockNotifications).invoke(null)
                ShadowNetworkManager.state.value = false
                flushEvents()
                verify(mockNotifications).invoke(argThat {
                    assertEquals("Network is not accessible", message)
                    assertEquals(Settings.ACTION_WIRELESS_SETTINGS, action!!.action)
                    true
                })
                ShadowNetworkManager.state.value = true
                flushEvents()
                verify(mockNotifications).invoke(null)
            }
            verifyNoMoreInteractions(mockNotifications)
        }
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
        runTest {
            val mockNotifications =
                mockCollectorOf(underTest.observeNotifications(noPermissionsUri))
            verify(mockNotifications).invoke(argThat {
                assertEquals("Tap to give access permission", message)
                requireNotNull(action).run {
                    assertEquals(noPermissionsIntent.action, action)
                    assertEquals(noPermissionsIntent.data, data)
                }
                true
            })
            verifyNoMoreInteractions(mockNotifications)
        }

        permissionQueryIntent = null
        runTest {
            val mockNotifications =
                mockCollectorOf(underTest.observeNotifications(noPermissionsUri))
            verify(mockNotifications).invoke(null)
            verifyNoMoreInteractions(mockNotifications)
        }
    }

    @Test
    fun `client progress exception`() = runTest {
        val error = Error("Client cancellation")
        listingCallback.stub {
            on { onProgress(any()) } doThrow error
        }
        // Error is transformed to Exception
        val ex = assertThrows<Exception> {
            underTest.list(slowUri, listingCallback)
        }
        assertEquals(error.message, ex.message)
        verify(listingCallback).onProgress(any())
    }

    @Test
    fun `client data exception`() = runTest {
        val error = Error("Client cancellation")
        listingCallback.stub {
            on { onFile(any()) } doThrow error
        }
        val ex = assertThrows<Error> {
            underTest.list(slowUri, listingCallback)
        }
        assertEquals(error.message, ex.message)
        verify(listingCallback, times(5)).onProgress(any())
        verify(listingCallback).onFile(any())
    }

    @Test
    fun `client cancellation from callback`() = runTest {
        lateinit var job: Job
        listingCallback.stub {
            // Due to testing environment, onFile is called after all onProgress calls
            on { onProgress(any()) } doAnswer {
                job.cancel()
            }
        }
        job = launch(SupervisorJob()) {
            underTest.list(slowUri, listingCallback)
        }
        job.join()
        verify(listingCallback).onProgress(any())
    }

    @Test
    fun `client cancellation`() = runTest {
        lateinit var job: Job
        launch(Dispatchers.IO) {
            delay(1000)
            job.cancel()
        }
        job = launch(SupervisorJob()) {
            underTest.list(hangingUri, listingCallback)
        }
        job.join()
    }
}

@Implements(NetworkManager::class)
class ShadowNetworkManager {

    companion object {
        val state = MutableStateFlow(true)

        @JvmStatic
        @Implementation
        fun initialize(ctx: Context) = Unit

        @JvmStatic
        @get:Implementation
        val networkAvailable: StateFlow<Boolean>
            get() = state
    }
}
