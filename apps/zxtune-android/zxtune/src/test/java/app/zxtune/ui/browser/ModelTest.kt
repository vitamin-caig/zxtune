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
import org.junit.Assert.assertNotEquals
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import java.util.concurrent.Executors
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
    private val async = Executors.newSingleThreadExecutor()
    private val modelClient = mock<Model.Client>()
    private lateinit var underTest: Model
    private val stateObserver = mock<Observer<Model.State>>()
    private val progressObserver = mock<Observer<Int?>>()
    private val notificationObserver = mock<Observer<Model.Notification?>>()

    @Before
    fun setUp() {
        underTest = Model(mock(), vfsClient, async).apply {
            setClient(modelClient)
            state.observeForever(stateObserver)
            progress.observeForever(progressObserver)
            notification.observeForever(notificationObserver)
        }
        reset(vfsClient, modelClient, stateObserver, progressObserver, notificationObserver)
    }

    @After
    fun tearDown() = verifyNoMoreInteractions(
        vfsClient, modelClient, stateObserver, progressObserver, notificationObserver
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

    private fun waitForIdle() = with(ReentrantLock()) {
        val signal = newCondition()
        withLock {
            async.submit {
                withLock { signal.signalAll() }
            }
            signal.awaitUninterruptibly()
        }
    }

    @Test
    fun `browse workflow`() {
        val thisThread = Thread.currentThread().id
        execute {
            browse(lazy {
                assertNotEquals(thisThread, Thread.currentThread().id)
                testUri
            })
        }
        inOrder(vfsClient, progressObserver) {
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
                    onProgress(Schema.Status.Progress(1, 2))
                    onProgress(Schema.Status.Progress(2, 2))
                    onFile(
                        Schema.Listing.File(
                            it.getArgument(0), "unused", "unused", "unused", null, null
                        )
                    )
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
                    onProgress(Schema.Status.Progress(1, 200))
                    onProgress(Schema.Status.Progress(100, 200))
                    onDir(Schema.Listing.Dir(it.getArgument(0), "unused", "unused", null, true))
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
                    onProgress(Schema.Status.Progress(5, 10))
                    onDir(Schema.Listing.Dir(it.getArgument(0), "unused", "unused", null, false))
                }
            }
            on { parents(any(), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ParentsCallback>(1)) {
                    testParents.forEach(this::feed)
                }
            }
            on { list(any(), any(), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ListingCallback>(1)) {
                    onProgress(Schema.Status.Progress(2, 10))
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
            verify(stateObserver).onChanged(matchState(testUri, testParents, testContent))
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
                    onDir(Schema.Listing.Dir(it.getArgument(0), "unused", "unused", null, false))
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
                verify(stateObserver).onChanged(matchState(parentUri, testParents.subList(0, 2)))
                verify(vfsClient).subscribeForNotifications(eq(parentUri), any())
                verify(notificationObserver).onChanged(notification)
                verify(progressObserver).onChanged(null)
            }
            testParents[0].uri.let { parentUri ->
                verify(progressObserver).onChanged(-1)
                verify(vfsClient).resolve(eq(parentUri), any(), any())
                verify(vfsClient).list(eq(parentUri), any(), any())
                verify(stateObserver).onChanged(matchState(parentUri, testParents.subList(0, 1)))
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
        setState(testParents, listOf())
        execute {
            reload()
        }
        inOrder(vfsClient, modelClient, progressObserver) {
            verify(progressObserver).onChanged(-1)
            verify(vfsClient).resolve(eq(testUri), any(), any())
            verify(progressObserver).onChanged(null)
        }
    }

    // Published state has no query field since it's for filtering
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
        inOrder(vfsClient, stateObserver, progressObserver) {
            verify(progressObserver).onChanged(-1)
            verify(stateObserver).onChanged(matchState(testUri, testParents))
            verify(vfsClient).search(eq(testUri), eq(noResultQuery), any(), any())
            verify(progressObserver).onChanged(null)

            // TODO: mock captures references of mutable object, so only last version is available
            verify(progressObserver).onChanged(-1)
            verify(stateObserver).onChanged(matchState(testUri, testParents, matchedContent))
            verify(vfsClient).search(eq(testUri), eq(testQuery), any(), any())
            verify(stateObserver, times(2)).onChanged(
                matchState(testUri, testParents, matchedContent)
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
            verify(stateObserver).onChanged(matchState(testUri, testParents, listOf()))
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

    @Test
    fun filtering() {
        setState(testParents, testContent)
        execute {
            filter("Le")
            waitForIdle()
            filter("le2")
            waitForIdle()
            filter("e23")
        }
        inOrder(stateObserver) {
            verify(stateObserver).onChanged(
                matchState(testUri, testParents, testContent.takeLast(2), "Le")
            )
            verify(stateObserver).onChanged(
                matchState(testUri, testParents, testContent.takeLast(1), "le2")
            )
            verify(stateObserver).onChanged(
                matchState(testUri, testParents, filter = "e23")
            )
        }
    }

    private fun setState(breadCrumbs: List<BreadcrumbsEntry>, entries: List<ListingEntry>) {
        underTest.mutableState.value = makeState(breadCrumbs, entries)
        clearInvocations(stateObserver)
    }
}

private fun makeState(
    breadCrumbs: List<BreadcrumbsEntry>, entry: List<ListingEntry>, filter: String? = null
) = Model.State().withContent(breadCrumbs, entry).run {
    filter?.let {
        withFilter(it)
    } ?: this
}

private fun matchState(
    uri: Uri,
    breadCrumbs: List<BreadcrumbsEntry>,
    entries: List<ListingEntry> = listOf(),
    filter: String = ""
) = argThat<Model.State> {
    uri == this.uri && breadCrumbs == this.breadcrumbs && entries == this.entries && filter == this.filter
}

private fun VfsProviderClient.ListingCallback.feed(entry: ListingEntry) = with(entry) {
    when (type) {
        ListingEntry.FILE -> onFile(
            Schema.Listing.File(
                uri, title, description, details.orEmpty(), tracks, cached
            )
        )

        ListingEntry.FOLDER -> onDir(Schema.Listing.Dir(uri, title, description, icon, false))
    }
}

private fun VfsProviderClient.ParentsCallback.feed(entry: BreadcrumbsEntry) = with(entry) {
    onObject(Schema.Parents.Object(uri, title, icon))
}
