package app.zxtune.ui.browser

import android.net.Uri
import android.os.CancellationSignal
import app.zxtune.TestUtils.mockCollectorOf
import app.zxtune.fs.provider.Schema
import app.zxtune.fs.provider.VfsProviderClient
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.asCoroutineDispatcher
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.delay
import kotlinx.coroutines.flow.emptyFlow
import kotlinx.coroutines.flow.flowOf
import kotlinx.coroutines.launch
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.TestScope
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.argThat
import org.mockito.kotlin.clearInvocations
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.doThrow
import org.mockito.kotlin.eq
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.stub
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner
import java.util.concurrent.Executors

@RunWith(RobolectricTestRunner::class)
class ModelTest {

    private val dispatcher = StandardTestDispatcher()

    private val testScope = TestScope()

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

    private val vfsClient = mock<VfsProviderClient> {
        on { observeNotifications(any()) } doReturn emptyFlow()
    }
    private val underTest = Model(mock(), vfsClient, dispatcher, dispatcher)
    private val stateObserver = testScope.mockCollectorOf(underTest.state)
    private val progressObserver = testScope.mockCollectorOf(underTest.progress)
    private val notificationsObserver = testScope.mockCollectorOf(underTest.notification)
    private val errorsObserver = testScope.mockCollectorOf(underTest.errors)
    private val playbacksObserver = testScope.mockCollectorOf(underTest.playbackEvents)
    private val allMocks = arrayOf(
        vfsClient,
        stateObserver,
        progressObserver,
        notificationsObserver,
        errorsObserver,
        playbacksObserver
    )

    @Before
    fun setUp() {
        // check state flows
        verify(stateObserver).invoke(argThat {
            assertTrue(breadcrumbs.isEmpty())
            assertTrue(entries.isEmpty())
            assertTrue(filter.isEmpty())
            assertEquals(Uri.EMPTY, this.uri)
            true
        })
        verify(progressObserver).invoke(null)
        verify(notificationsObserver).invoke(null)
        verifyNoMoreInteractions(*allMocks)
        clearInvocations(*allMocks)
    }

    @After
    fun tearDown() = verifyNoMoreInteractions(*allMocks)

    private fun execute(cmd: Model.() -> Unit) {
        underTest.cmd()
        dispatcher.scheduler.advanceUntilIdle()
    }

    @Test
    fun `initial state`() {
        // Just check Before/After consistence
        execute {}
    }

    @Test
    fun `browse not resolved`() {
        execute {
            browse(testUri)
        }
        inOrder(vfsClient, progressObserver) {
            verify(progressObserver).invoke(-1)
            verifyBlocking(vfsClient) {
                resolve(eq(testUri), any())
            }
            verify(progressObserver).invoke(null)
        }
    }

    @Test
    fun `browseParent initial`() {
        execute {
            browseParent()
        }
        inOrder(vfsClient, progressObserver) {
            verify(progressObserver).invoke(-1)
            verifyBlocking(vfsClient) {
                resolve(eq(Uri.EMPTY), any())
            }
            verify(progressObserver).invoke(null)
        }
    }

    @Test
    fun `search initial`() {
        execute {
            search(testQuery)
        }
        inOrder(vfsClient, progressObserver) {
            verify(progressObserver).invoke(-1)
            verifyBlocking(vfsClient) {
                search(eq(Uri.EMPTY), eq(testQuery), any())
            }
            verify(progressObserver).invoke(null)
        }
    }

    @Test
    fun `browse file`() {
        vfsClient.stub {
            onBlocking { resolve(any(), any()) } doAnswer {
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
        inOrder(vfsClient, progressObserver, playbacksObserver) {
            verify(progressObserver).invoke(-1)
            verifyBlocking(vfsClient) {
                resolve(eq(testUri), any())
            }
            verify(progressObserver).invoke(50)
            verify(progressObserver).invoke(100)
            verify(playbacksObserver).invoke(testUri)
            verify(progressObserver).invoke(null)
        }
    }

    @Test
    fun `browse dir with feed`() {
        vfsClient.stub {
            onBlocking { resolve(any(), any()) } doAnswer {
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
        inOrder(vfsClient, progressObserver, playbacksObserver) {
            verify(progressObserver).invoke(-1)
            verifyBlocking(vfsClient) { resolve(eq(testUri), any()) }
            verify(progressObserver).invoke(0)
            verify(progressObserver).invoke(50)
            verify(playbacksObserver).invoke(testUri)
            verify(progressObserver).invoke(null)
        }
    }

    @Test
    fun `browse dir`() {
        vfsClient.stub {
            onBlocking { resolve(any(), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ListingCallback>(1)) {
                    onProgress(Schema.Status.Progress(5, 10))
                    onDir(Schema.Listing.Dir(it.getArgument(0), "unused", "unused", null, false))
                }
            }
            onBlocking { parents(any(), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ParentsCallback>(1)) {
                    testParents.forEach(this::feed)
                }
            }
            onBlocking { list(any(), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ListingCallback>(1)) {
                    onProgress(Schema.Status.Progress(2, 10))
                    testContent.forEach(this::feed)
                }
            }
        }
        execute {
            browse(testUri)
        }
        inOrder(vfsClient, progressObserver, stateObserver, notificationsObserver) {
            verify(progressObserver).invoke(-1)
            verifyBlocking(vfsClient) { resolve(eq(testUri), any()) }
            verify(progressObserver).invoke(50)
            verifyBlocking(vfsClient) { parents(eq(testUri), any()) }
            verifyBlocking(vfsClient) { list(eq(testUri), any()) }
            verify(progressObserver).invoke(20)
            verify(vfsClient).observeNotifications(testUri)
            verify(progressObserver).invoke(null)
            verify(stateObserver).invoke(matchState(testUri, testParents, testContent))
        }
    }

    @Test
    fun `browse failed`() {
        val err = IllegalArgumentException("Fail")
        vfsClient.stub {
            onBlocking { resolve(any(), any()) } doThrow err
        }
        execute {
            browse(testUri)
        }
        inOrder(vfsClient, progressObserver, stateObserver, errorsObserver) {
            verify(progressObserver).invoke(-1)
            verifyBlocking(vfsClient) { resolve(eq(testUri), any()) }
            verify(progressObserver).invoke(null)
            verify(errorsObserver).invoke(requireNotNull(err.message))
        }
    }

    @Test
    fun `browseParent with unresolvable state`() {
        setContent(testParents, listOf())
        execute {
            browseParent()
        }
        inOrder(vfsClient, stateObserver, progressObserver) {
            // setContent
            testParents.last().uri.let { parentUri ->
                verify(vfsClient).observeNotifications(parentUri)
                verify(stateObserver).invoke(matchState(parentUri, testParents))
            }
            verify(progressObserver).invoke(-1)
            verifyBlocking(vfsClient) { resolve(eq(testParents[1].uri), any()) }
            verifyBlocking(vfsClient) { resolve(eq(testParents[0].uri), any()) }
            verify(progressObserver).invoke(null)
        }
    }

    @Test
    fun `browseParent with good state`() {
        setContent(testParents, listOf())
        vfsClient.stub {
            onBlocking { resolve(any(), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ListingCallback>(1)) {
                    onDir(Schema.Listing.Dir(it.getArgument(0), "unused", "unused", null, false))
                }
            }
            on { observeNotifications(any()) } doAnswer {
                flowOf(
                    Schema.Notifications.Object("Notification for ${it.getArgument<Uri>(0)}", null)
                )
            }
        }
        execute {
            browseParent()
        }
        execute {
            browseParent()
        }
        inOrder(
            vfsClient, stateObserver, progressObserver, notificationsObserver
        ) {
            // setContent
            testParents.last().uri.let { parentUri ->
                verify(vfsClient).observeNotifications(parentUri)
                verify(stateObserver).invoke(matchState(parentUri, testParents))
            }
            testParents[1].uri.let { parentUri ->
                verify(progressObserver).invoke(-1)
                verifyBlocking(vfsClient) { resolve(eq(parentUri), any()) }
                verifyBlocking(vfsClient) { list(eq(parentUri), any()) }
                verify(vfsClient).observeNotifications(parentUri)
                verify(notificationsObserver).invoke(argThat {
                    message == "Notification for $parentUri" && action == null
                })
                verify(progressObserver).invoke(null)
                verify(stateObserver).invoke(matchState(parentUri, testParents.subList(0, 2)))
            }
            testParents[0].uri.let { parentUri ->
                verify(progressObserver).invoke(-1)
                verifyBlocking(vfsClient) { resolve(eq(parentUri), any()) }
                verifyBlocking(vfsClient) { list(eq(parentUri), any()) }
                verify(vfsClient).observeNotifications(parentUri)
                verify(notificationsObserver).invoke(argThat {
                    message == "Notification for $parentUri" && action == null
                })
                verify(progressObserver).invoke(null)
                verify(stateObserver).invoke(matchState(parentUri, testParents.subList(0, 1)))
            }
        }
    }

    @Test
    fun `browseParent with single dir`() {
        val parents = testParents.subList(0, 1)
        setContent(parents, listOf())
        execute {
            browseParent()
        }
        inOrder(vfsClient, progressObserver, stateObserver) {
            // setContent
            parents.last().uri.let { parentUri ->
                verify(vfsClient).observeNotifications(parentUri)
                verify(stateObserver).invoke(matchState(parentUri, parents))
            }
            verify(progressObserver).invoke(-1)
            verifyBlocking(vfsClient) { resolve(eq(Uri.EMPTY), any()) }
            verify(progressObserver).invoke(null)
        }
    }

    @Test
    fun `browseParent failed to resolve`() {
        val err = IllegalArgumentException("Fail")
        vfsClient.stub {
            onBlocking { resolve(any(), any()) } doThrow err
        }
        `browseParent with unresolvable state`()
    }

    // Published state has no query field since it's for filtering
    @Test
    fun `search with state`() {
        val noResultQuery = "Unused"
        val matchedContent = listOf(testContent[1], testContent[2])
        vfsClient.stub {
            onBlocking { search(any(), eq(testQuery), any()) } doAnswer {
                with(it.getArgument<VfsProviderClient.ListingCallback>(2)) {
                    CoroutineScope(dispatcher).launch {
                        matchedContent.forEach { entry ->
                            delay(1000)
                            feed(entry)
                        }
                    }
                    dispatcher.scheduler.advanceUntilIdle()
                }
            }
        }
        setContent(testParents, testContent)
        execute {
            search(noResultQuery)
        }
        execute {
            search(testQuery)
        }
        inOrder(vfsClient, stateObserver, progressObserver) {
            // setContent
            testParents.last().uri.let { parentUri ->
                verify(vfsClient).observeNotifications(parentUri)
                verify(stateObserver).invoke(matchState(parentUri, testParents, testContent))
            }
            verify(progressObserver).invoke(-1)
            verifyBlocking(vfsClient) { search(eq(testUri), eq(noResultQuery), any()) }
            verify(progressObserver).invoke(null)
            verify(stateObserver).invoke(matchState(testUri, testParents))

            verify(progressObserver).invoke(-1)
            verifyBlocking(vfsClient) { search(eq(testUri), eq(testQuery), any()) }
            verify(stateObserver).invoke(matchState(testUri, testParents))
            verify(stateObserver).invoke(
                matchState(
                    testUri, testParents, matchedContent.subList(0, 1)
                )
            )
            verify(stateObserver).invoke(matchState(testUri, testParents, matchedContent))
            verify(progressObserver).invoke(null)
        }
    }

    @Test
    fun `search failed`() {
        val err = IllegalArgumentException("Fail")
        vfsClient.stub {
            onBlocking { search(any(), any(), any()) } doThrow err
        }
        setContent(testParents, testContent)
        execute {
            search(testQuery)
        }
        inOrder(vfsClient, stateObserver, progressObserver, errorsObserver) {
            // setContent
            testParents.last().uri.let { parentUri ->
                verify(vfsClient).observeNotifications(parentUri)
                verify(stateObserver).invoke(matchState(parentUri, testParents, testContent))
            }
            verify(progressObserver).invoke(-1)
            verifyBlocking(vfsClient) { search(eq(testUri), eq(testQuery), any()) }
            verify(progressObserver).invoke(null)
            verify(errorsObserver).invoke(checkNotNull(err.message))
            verify(stateObserver).invoke(matchState(testUri, testParents))
            verify(stateObserver).invoke(matchState(testUri, testParents, testContent))
        }
    }

    @Test
    fun `queued requests with cancellation`() {
        // Required in order to hang vfsClient in separate thread
        val parallelDispatcher = Executors.newSingleThreadExecutor().asCoroutineDispatcher()
        val model = Model(mock(), vfsClient, parallelDispatcher, parallelDispatcher)
        val progress = testScope.mockCollectorOf(model.progress)

        val slowUri = Uri.parse("slow://")
        val chan = Channel<Unit>()
        vfsClient.stub {
            onBlocking { resolve(eq(slowUri), any()) } doAnswer {
                chan.trySend(Unit)
                with(it.getArgument<CancellationSignal>(2)) {
                    while (true) {
                        throwIfCanceled()
                        Thread.sleep(100)
                    }
                }
            }
            onBlocking { resolve(eq(testUri), any()) } doAnswer {
                chan.trySend(Unit)
                Unit
            }
        }
        testScope.runTest {
            model.browse(slowUri)
            chan.receive()
            model.browse(testUri)
            chan.receive()
        }
        inOrder(vfsClient, progress) {
            // initial
            verify(progress).invoke(null)
            // slow
            verify(progress).invoke(-1)
            verifyBlocking(vfsClient) { resolve(eq(slowUri), any()) }
            verify(progress).invoke(null)
            // next op
            verify(progress).invoke(-1)
            verifyBlocking(vfsClient) { resolve(eq(testUri), any()) }
            verify(progress).invoke(null)
        }
        verifyNoMoreInteractions(progressObserver)
    }

    @Test
    fun filtering() {
        setContent(testParents, testContent)
        execute {
            filter = "Le"
        }
        execute {
            filter = "le2"
        }
        execute {
            filter = "e23"
        }
        inOrder(vfsClient, stateObserver) {
            // setContent
            testParents.last().uri.let { parentUri ->
                verify(vfsClient).observeNotifications(parentUri)
                verify(stateObserver).invoke(matchState(parentUri, testParents, testContent))
            }

            verify(stateObserver).invoke(
                matchState(testUri, testParents, testContent.takeLast(2), "Le")
            )
            verify(stateObserver).invoke(
                matchState(testUri, testParents, testContent.takeLast(1), "le2")
            )
            verify(stateObserver).invoke(
                matchState(testUri, testParents, filter = "e23")
            )
        }
    }

    private fun setContent(
        breadCrumbs: List<BreadcrumbsEntry>, entries: List<ListingEntry>
    ) {
        CoroutineScope(dispatcher).launch {
            underTest.updateContent(breadCrumbs, entries)
        }
        dispatcher.scheduler.advanceUntilIdle()
    }
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
    details?.let {
        onFile(Schema.Listing.File(uri, title, description, it, null, null))
    } ?: onDir(Schema.Listing.Dir(uri, title, description, mainIcon, false))
}

private fun VfsProviderClient.ParentsCallback.feed(entry: BreadcrumbsEntry) = with(entry) {
    onObject(Schema.Parents.Object(uri, title, icon))
}
