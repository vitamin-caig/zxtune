package app.zxtune.ui.browser

import android.net.Uri
import android.os.CancellationSignal
import androidx.arch.core.executor.testing.InstantTaskExecutorRule
import androidx.lifecycle.Observer
import app.zxtune.Releaseable
import app.zxtune.fs.provider.Schema
import app.zxtune.fs.provider.VfsProviderClient
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import java.util.concurrent.locks.ReentrantLock
import kotlin.concurrent.withLock

@RunWith(RobolectricTestRunner::class)
class ModelTest {

    @get:Rule
    val instantTaskExecutorRule = InstantTaskExecutorRule()

    private val testUri = Uri.parse("scheme://host/path?query#fragment")
    private val testQuery = "file"

    private val testParents = listOf(
        BreadcrumbsEntry(Uri.parse("scheme://host"), "RootDir", null),
        BreadcrumbsEntry(Uri.parse("scheme://host/path"), "ParentDir", 123),
        BreadcrumbsEntry(testUri, "TestDir", 456)
    )
    private val testContent = listOf(
        ListingEntry.makeFolder(Uri.EMPTY, "Dir", "Nested dir", 234),
        ListingEntry.makeFile(Uri.EMPTY, "File", "Nested file", "Unused"),
        ListingEntry.makeFile(Uri.EMPTY, "File2", "Nested file2", "Unused")
    )

    private val vfsClient = mock<VfsProviderClient>()
    private val modelClient = mock<Model.Client>()
    private lateinit var underTest: Model
    private val stateObserver = mock<Observer<Model.State>>()
    private val progressObserver = mock<Observer<Int?>>()
    private val notificationObserver = mock<Observer<Model.Notification?>>()

    @Before
    fun setUp() {
        underTest = Model(mock(), vfsClient).apply {
            setClient(modelClient)
            state.observeForever(stateObserver)
            progress.observeForever(progressObserver)
            notification.observeForever(notificationObserver)

        }
        reset(vfsClient, modelClient, stateObserver, progressObserver, notificationObserver)
    }

    @After
    fun tearDown() =
        verifyNoMoreInteractions(
            vfsClient,
            modelClient,
            stateObserver,
            progressObserver,
            notificationObserver
        )

    @Test
    fun `initial state`() = with(underTest) {
        assertEquals(null, mutableState.value)
        assertEquals(null, progress.value)
        assertEquals(null, notification.value)
    }

    private fun execute(cmd: Model.() -> Unit) {
        with(underTest) {
            cmd(this)
            waitForIdle()
        }
    }

    @Test
    fun `browseAsync workflow`() {
        execute {
            browseAsync {
                testUri
            }
        }
        inOrder(vfsClient, modelClient, progressObserver) {
            verify(progressObserver).onChanged(-1)
            verify(vfsClient).resolve(eq(testUri), any(), any())
            verify(progressObserver).onChanged(null)
        }
    }

    @Test
    fun `browse not resolved`() {
        execute {
            browse(testUri)
        }
        inOrder(vfsClient, modelClient, progressObserver) {
            verify(progressObserver).onChanged(-1)
            verify(vfsClient).resolve(eq(testUri), any(), any())
            verify(progressObserver).onChanged(null)
        }
    }

    @Test
    fun `browseParent initial`() {
        execute {
            browseParent()
        }
        inOrder(vfsClient, modelClient, progressObserver) {
            verify(progressObserver).onChanged(-1)
            verify(vfsClient).resolve(eq(Uri.EMPTY), any(), any())
            verify(progressObserver).onChanged(null)
        }
    }

    @Test
    fun `reload initial`() {
        execute {
            reload()
        }
    }

    @Test
    fun `search initial`() {
        execute {
            search(testQuery)
        }
    }

    @Test
    fun `browse file`() {
        vfsClient.stub {
            on { resolve(any(), any(), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ListingCallback>(1)) {
                    onProgress(1, 2)
                    onProgress(2, 2)
                    onFile(it.getArgument(0), "unused", "unused", "unused", null, null)
                }
            }
        }
        execute {
            browse(testUri)
        }
        inOrder(vfsClient, modelClient, progressObserver, modelClient) {
            verify(progressObserver).onChanged(-1)
            verify(vfsClient).resolve(eq(testUri), any(), any())
            verify(progressObserver).onChanged(50)
            verify(progressObserver).onChanged(100)
            verify(modelClient).onFileBrowse(testUri)
            verify(progressObserver).onChanged(null)
        }
    }

    @Test
    fun `browse dir with feed`() {
        vfsClient.stub {
            on { resolve(any(), any(), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ListingCallback>(1)) {
                    onProgress(1, 200)
                    onProgress(100, 200)
                    onDir(it.getArgument(0), "unused", "unused", null, true)
                }
            }
        }
        execute {
            browse(testUri)
        }
        inOrder(vfsClient, modelClient, progressObserver, modelClient) {
            verify(progressObserver).onChanged(-1)
            verify(vfsClient).resolve(eq(testUri), any(), any())
            verify(progressObserver).onChanged(0)
            verify(progressObserver).onChanged(50)
            verify(modelClient).onFileBrowse(testUri)
            verify(progressObserver).onChanged(null)
        }
    }

    @Test
    fun `browse dir`() {
        vfsClient.stub {
            on { resolve(any(), any(), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ListingCallback>(1)) {
                    onProgress(5, 10)
                    onDir(it.getArgument(0), "unused", "unused", null, false)
                }
            }
            on { parents(any(), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ParentsCallback>(1)) {
                    testParents.forEach(this::feed)
                }
            }
            on { list(any(), any(), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ListingCallback>(1)) {
                    onProgress(2, 10)
                    testContent.forEach(this::feed)
                }
            }
            on { subscribeForNotifications(any(), any()) } doAnswer {
                it.getArgument<(Schema.Notifications.Object?) -> Unit>(1).invoke(null)
                mock()
            }
        }
        execute {
            browse(testUri)
        }
        inOrder(vfsClient, modelClient, progressObserver, stateObserver, notificationObserver) {
            verify(progressObserver).onChanged(-1)
            verify(vfsClient).resolve(eq(testUri), any(), any())
            verify(progressObserver).onChanged(50)
            verify(vfsClient).parents(eq(testUri), any())
            verify(vfsClient).list(eq(testUri), any(), any())
            verify(progressObserver).onChanged(20)
            verify(stateObserver).onChanged(Model.State(testUri, testParents, testContent))
            verify(vfsClient).subscribeForNotifications(eq(testUri), any())
            verify(notificationObserver).onChanged(null)
            verify(progressObserver).onChanged(null)
        }
    }

    @Test
    fun `browse failed`() {
        val err = Exception("Fail")
        vfsClient.stub {
            on { resolve(any(), any(), any()) } doThrow err
        }
        execute {
            browse(testUri)
        }
        inOrder(vfsClient, modelClient, progressObserver, stateObserver) {
            verify(progressObserver).onChanged(-1)
            verify(vfsClient).resolve(eq(testUri), any(), any())
            verify(modelClient).onError(err.message!!)
            verify(progressObserver).onChanged(null)
        }
    }

    @Test
    fun `browseParent with unresolvable state`() {
        setState(testParents, listOf())
        execute {
            browseParent()
        }
        inOrder(vfsClient, modelClient, progressObserver) {
            verify(progressObserver).onChanged(-1)
            verify(vfsClient).resolve(eq(testParents[1].uri), any(), any())
            verify(vfsClient).resolve(eq(testParents[0].uri), any(), any())
            verify(progressObserver).onChanged(null)
        }
    }

    @Test
    fun `browseParent with good state`() {
        setState(testParents, listOf())
        val notification = Model.Notification("message", null)
        val notificationHandle = mock<Releaseable>()
        vfsClient.stub {
            on { resolve(any(), any(), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ListingCallback>(1)) {
                    onDir(it.getArgument(0), "unused", "unused", null, false)
                }
            }
            on { subscribeForNotifications(any(), any()) } doAnswer {
                it.getArgument<(Schema.Notifications.Object?) -> Unit>(1)
                    .invoke(Schema.Notifications.Object(notification.message, notification.action))
                notificationHandle
            }
        }
        execute {
            browseParent()
            waitForIdle()
            browseParent()
        }
        inOrder(
            vfsClient,
            modelClient,
            stateObserver,
            progressObserver,
            notificationObserver,
            notificationHandle
        ) {
            testParents[1].uri.let { parentUri ->
                verify(progressObserver).onChanged(-1)
                verify(vfsClient).resolve(eq(parentUri), any(), any())
                verify(vfsClient).list(eq(parentUri), any(), any())
                verify(stateObserver).onChanged(
                    Model.State(
                        parentUri,
                        testParents.subList(0, 2),
                        listOf()
                    )
                )
                verify(vfsClient).subscribeForNotifications(eq(parentUri), any())
                verify(notificationObserver).onChanged(notification)
                verify(progressObserver).onChanged(null)
            }
            testParents[0].uri.let { parentUri ->
                verify(progressObserver).onChanged(-1)
                verify(vfsClient).resolve(eq(parentUri), any(), any())
                verify(vfsClient).list(eq(parentUri), any(), any())
                verify(stateObserver).onChanged(
                    Model.State(
                        parentUri,
                        testParents.subList(0, 1),
                        listOf()
                    )
                )
                verify(notificationHandle).release()
                verify(vfsClient).subscribeForNotifications(eq(parentUri), any())
                verify(notificationObserver).onChanged(notification)
                verify(progressObserver).onChanged(null)
            }
        }
    }

    @Test
    fun `browseParent with single dir`() {
        setState(testParents.subList(0, 1), listOf())
        execute {
            browseParent()
        }
        inOrder(vfsClient, modelClient, progressObserver) {
            verify(progressObserver).onChanged(-1)
            verify(vfsClient).resolve(eq(Uri.EMPTY), any(), any())
            verify(progressObserver).onChanged(null)
        }
    }

    @Test
    fun `browseParent failed to resolve`() {
        val err = Exception("Fail")
        vfsClient.stub {
            on { resolve(any(), any(), any()) } doThrow err
        }
        `browseParent with unresolvable state`()
    }

    @Test
    fun `reload with state`() {
        setState(listOf(), listOf())
        execute {
            reload()
        }
        inOrder(vfsClient, modelClient, progressObserver) {
            verify(progressObserver).onChanged(-1)
            verify(vfsClient).resolve(eq(testUri), any(), any())
            verify(progressObserver).onChanged(null)
        }
    }

    @Test
    fun `search with state`() {
        val noResultQuery = "Unused"
        val matchedContent = listOf(testContent[1], testContent[2])
        vfsClient.stub {
            on { search(any(), eq(testQuery), any(), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ListingCallback>(2)) {
                    matchedContent.forEach(this::feed)
                }
            }
        }
        setState(testParents, listOf())
        execute {
            search(noResultQuery)
        }
        execute {
            search(testQuery)
        }
        inOrder(vfsClient, modelClient, stateObserver, progressObserver) {
            verify(progressObserver).onChanged(-1)
            verify(stateObserver).onChanged(
                Model.State(
                    testUri,
                    testParents,
                    listOf(),
                    noResultQuery
                )
            )
            verify(vfsClient).search(eq(testUri), eq(noResultQuery), any(), any())
            verify(progressObserver).onChanged(null)

            // TODO: mock captures references of mutable object, so only last version is available
            verify(progressObserver).onChanged(-1)
            verify(stateObserver).onChanged(
                Model.State(
                    testUri,
                    testParents,
                    matchedContent,
                    testQuery
                )
            )
            verify(vfsClient).search(eq(testUri), eq(testQuery), any(), any())
            verify(stateObserver, times(2)).onChanged(
                Model.State(
                    testUri,
                    testParents,
                    matchedContent,
                    testQuery
                )
            )
            verify(progressObserver).onChanged(null)
        }
        verifyNoMoreInteractions(stateObserver)
    }

    @Test
    fun `search failed`() {
        val err = Exception("Fail")
        vfsClient.stub {
            on { search(any(), any(), any(), any()) } doThrow err
        }
        setState(testParents, listOf())
        execute {
            search(testQuery)
        }
        inOrder(vfsClient, modelClient, stateObserver, progressObserver) {
            verify(progressObserver).onChanged(-1)
            verify(stateObserver).onChanged(Model.State(testUri, testParents, listOf(), testQuery))
            verify(vfsClient).search(eq(testUri), eq(testQuery), any(), any())
            verify(modelClient).onError(err.message!!)
            verify(progressObserver).onChanged(null)
        }
    }

    @Test
    fun `no notifications`() {
        execute {

        }
    }

    @Test
    fun `queued requests with cancellation`() {
        val slowUri = Uri.parse("slow://")
        val lock = ReentrantLock()
        val signal = lock.newCondition()
        vfsClient.stub {
            on { resolve(eq(slowUri), any(), any()) } doAnswer {
                lock.withLock {
                    signal.signal()
                }
                with(it.getArgument<CancellationSignal>(2)) {
                    throwIfCanceled()
                    Thread.sleep(1000)
                }
            }
        }
        execute {
            browse(slowUri)
            lock.withLock {
                signal.await()
            }
            browse(testUri)
        }
        inOrder(vfsClient, modelClient, progressObserver) {
            verify(progressObserver).onChanged(-1)
            verify(vfsClient).resolve(eq(slowUri), any(), any())
            //verify(progressObserver).onChanged(null)

            verify(progressObserver).onChanged(-1)
            verify(vfsClient).resolve(eq(testUri), any(), any())
            verify(progressObserver).onChanged(null)
        }
    }

    private fun setState(breadCrumbs: List<BreadcrumbsEntry>, entries: List<ListingEntry>) {
        underTest.mutableState.value = Model.State(testUri, breadCrumbs, entries)
        clearInvocations(stateObserver)
    }
}

private fun VfsProviderClient.ListingCallback.feed(entry: ListingEntry) = with(entry) {
    when (type) {
        ListingEntry.FILE -> onFile(uri, title, description, details.orEmpty(), tracks, cached)
        ListingEntry.FOLDER -> onDir(uri, title, description, icon, false)
    }
}

private fun VfsProviderClient.ParentsCallback.feed(entry: BreadcrumbsEntry) = with(entry) {
    onObject(uri, title, icon)
}
