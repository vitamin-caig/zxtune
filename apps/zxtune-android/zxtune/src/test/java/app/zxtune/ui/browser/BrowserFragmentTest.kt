package app.zxtune.ui.browser

import android.net.Uri
import android.view.ViewGroup
import android.widget.Button
import android.widget.ProgressBar
import android.widget.SearchView
import androidx.core.view.get
import androidx.fragment.app.testing.FragmentScenario
import androidx.recyclerview.widget.RecyclerView
import app.zxtune.R
import app.zxtune.TestUtils.construct
import app.zxtune.TestUtils.constructedInstance
import app.zxtune.TestUtils.flushEvents
import app.zxtune.ui.AsyncDifferInMainThreadRule
import app.zxtune.ui.MainDispatcherRule
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.runBlocking
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNotEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.clearInvocations
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.reset
import org.mockito.kotlin.stub
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyBlocking
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class BrowserFragmentTest {

    @get:Rule
    val mainThreadDiffer = AsyncDifferInMainThreadRule()

    private val dispatcher = StandardTestDispatcher()

    // to inject to fragment model operations
    @get:Rule
    val mainDispatcher = MainDispatcherRule(dispatcher)

    private val persistentState = mock<State>()

    private val mocks
        get() = arrayOf(persistentState)

    @Before
    fun setUp() {
        State.create = { persistentState }
        reset(*mocks)
    }

    @After
    fun tearDown() = verifyNoMoreInteractions(*mocks)

    private fun startScenario() = FragmentScenario.launchInContainer(
        fragmentClass = BrowserFragment::class.java,
        themeResId = R.style.CustomTheme,
    )

    @Test
    fun `first start no data`() = runTest {
        // check basic subscriptions
        val stateFlow = mock<Flow<Model.State>>()
        val progressFlow = mock<Flow<Int?>>()
        val notificationsFlow = mock<Flow<Model.Notification?>>()
        val errorsFlow = mock<Flow<String>>()
        val playbackEventsFlow = mock<Flow<Uri>>()
        val path = Uri.parse("")
        persistentState.stub {
            onBlocking { getCurrentPath() } doReturn path
        }
        construct<Model> {
            on { state } doReturn stateFlow
            on { progress } doReturn progressFlow
            on { notification } doReturn notificationsFlow
            on { errors } doReturn errorsFlow
            on { playbackEvents } doReturn playbackEventsFlow
            on { initialize(any()) } doAnswer {
                it.getArgument<Uri>(0)
                Unit
            }
        }.use { modelConstruction ->
            startScenario().onFragment {
                flushEvents()
                val model = modelConstruction.constructedInstance
                verify(model).initialize(any())
                verifyBlocking(persistentState) {
                    getCurrentPath()
                }
                verify(model).state
                verify(model).progress
                verify(model).notification
                verify(model).errors
                verify(model).playbackEvents
                runBlocking {
                    verify(stateFlow).collect(any())
                    verify(progressFlow).collect(any())
                    verify(notificationsFlow).collect(any())
                    verify(errorsFlow).collect(any())
                    verify(playbackEventsFlow).collect(any())
                }
                //verify(model).onCleared()
            }.close()
            verifyNoMoreInteractions(
                stateFlow, progressFlow, notificationsFlow, errorsFlow, playbackEventsFlow
            )
        }
    }

    @Test
    fun `with content and state`() = runTest {
        val listingState = makeState(3, 2, 3)
        persistentState.stub {
            onBlocking { getCurrentPath() } doReturn listingState.uri
            onBlocking { updateCurrentPath(any()) } doReturn 0
        }
        construct<Model> {
            on { state } doReturn MutableStateFlow(listingState)
            on { progress } doReturn mock()
            on { notification } doReturn mock()
            on { errors } doReturn mock()
            on { playbackEvents } doReturn mock()
        }.use { modelConstruction ->
            lateinit var model: Model
            startScenario().onFragment {
                flushEvents()
                model = modelConstruction.constructedInstance
                clearInvocations(*mocks, model)
                // TODO: rework back pressing for tablets
                it.onTabVisibilityChanged(true)
                it.view!!.run {
                    findViewById<Button>(R.id.browser_roots).callOnClick()
                    findViewById<RecyclerView>(R.id.browser_breadcrumb).let { rv ->
                        assertEquals(3, rv.childCount)
                        (rv[0] as Button).run {
                            assertEquals("", text)
                            assertNotEquals(null, compoundDrawables[0])
                            callOnClick()
                        }
                        (rv[2] as Button).run {
                            assertEquals(listingState.breadcrumbs[2].title, text)
                            assertEquals(null, compoundDrawables[0])
                            callOnClick()
                        }
                    }
                    val listing = findViewById<RecyclerView>(R.id.browser_content)
                    assertEquals(5, listing.childCount)
                    // TODO: investigate RV click emulation possibility
                    findViewById<SearchView>(R.id.browser_search).let { view ->
                        assertEquals(ViewGroup.LayoutParams.WRAP_CONTENT, view.layoutParams.width)
                        view.isIconified = false
                        assertEquals(ViewGroup.LayoutParams.MATCH_PARENT, view.layoutParams.width)
                        it.requireActivity().onBackPressed()
                        assertEquals(true, view.isIconified)
                        assertEquals(ViewGroup.LayoutParams.WRAP_CONTENT, view.layoutParams.width)
                        flushEvents()
                    }
                }
                it.requireActivity().onBackPressed()
                flushEvents()
            }.close()
            flushEvents()
            inOrder(persistentState, model) {
                verify(model).browse(Uri.EMPTY)
                verify(model).browse(listingState.breadcrumbs[0].uri)
                verify(model).browse(listingState.breadcrumbs[2].uri)
                // search
                verify(model).cancelSearch()
                verify(model).browseParent()
                // at close()
                verifyBlocking(persistentState) {
                    updateCurrentPosition(0)
                }
                //verify(model).onClear()
            }
            //verifyNoMoreInteractions(modelConstruction.constructedInstance)
        }
    }

    @Test
    fun `progress state`() = runTest {
        val operationProgress = MutableStateFlow<Int?>(null)
        persistentState.stub {
            onBlocking { getCurrentPath() } doReturn Uri.EMPTY
        }
        construct<Model> {
            on { state } doReturn mock()
            on { progress } doReturn operationProgress
            on { notification } doReturn mock()
            on { errors } doReturn mock()
            on { playbackEvents } doReturn mock()
        }.use {
            startScenario().onFragment {
                clearInvocations(*mocks)
                it.view!!.findViewById<ProgressBar>(R.id.browser_loading).run {
                    operationProgress.value = null
                    flushEvents()
                    assertEquals(0, progress)
                    assertEquals(false, isIndeterminate)

                    operationProgress.value = -1
                    flushEvents()
                    assertEquals(0, progress)
                    assertEquals(true, isIndeterminate)

                    operationProgress.value = 0
                    flushEvents()
                    assertEquals(0, progress)
                    assertEquals(false, isIndeterminate)

                    operationProgress.value = 100
                    flushEvents()
                    assertEquals(100, progress)
                    assertEquals(false, isIndeterminate)
                }
            }.close()
        }
        verifyBlocking(persistentState) {
            getCurrentPath()
        }
    }

    @Test
    fun `search filtering`() = runTest {
        val listingState = MutableStateFlow(makeState(5, 3, 4))
        persistentState.stub {
            onBlocking { getCurrentPath() } doReturn listingState.value.uri
            onBlocking { updateCurrentPath(any()) } doReturn 0
        }
        construct<Model> {
            on { state } doReturn listingState
            on { progress } doReturn mock()
            on { notification } doReturn mock()
            on { errors } doReturn mock()
            on { playbackEvents } doReturn mock()
            on { filter } doAnswer {
                listingState.value.filter
            }
            on { filter = any() } doAnswer {
                listingState.value = listingState.value.withFilter(it.getArgument(0))
            }
        }.use { modelConstruction ->
            startScenario().onFragment {
                flushEvents()
                val model = modelConstruction.constructedInstance
                clearInvocations(model)
                it.view!!.run {
                    val adapter = findViewById<RecyclerView>(R.id.browser_content).adapter!!
                    findViewById<SearchView>(R.id.browser_search).let { view ->
                        assertEquals(7, adapter.itemCount)

                        assertTrue(view.requestFocus())
                        view.setQuery("Dir", false)
                        flushEvents()
                        assertEquals(3, adapter.itemCount)

                        view.setQuery("", false)
                        flushEvents()
                        assertEquals(7, adapter.itemCount)

                        view.setQuery("File ", false)
                        flushEvents()
                        assertEquals(4, adapter.itemCount)

                        view.setQuery("Model query", true)
                    }
                }
                inOrder(model) {
                    verify(model).filter = "Dir"
                    verify(model).filter = ""
                    verify(model).filter = "File "
                    verify(model).filter = "Model query"
                    verify(model).search("Model query")
                    //verify(model).onClear()
                }
                //verifyNoMoreInteractions(model)
            }.close()
            flushEvents()
        }
        inOrder(persistentState) {
            verifyBlocking(persistentState) {
                getCurrentPath()
            }
            verifyBlocking(persistentState) {
                updateCurrentPath(listingState.value.uri)
            }
            verifyBlocking(persistentState) {
                updateCurrentPosition(0)
            }
        }
    }

    /* TODO: investigate for touch events emulating
    @Test
    fun `selection mode`() {
    }
    */

    companion object {
        // Resources identifiers cannot be const values
        private fun parentIcon() = R.drawable.ic_launcher
        private fun folderIcon() = R.drawable.ic_browser_vfs_local

        private fun makeState(
            depth: Int,
            dirs: Int,
            files: Int,
        ): Model.State {
            val breadcrumbs = ArrayList<BreadcrumbsEntry>(depth).apply {
                val builder = Uri.Builder().scheme("scheme")
                for (idx in 0 until depth) {
                    add(BreadcrumbsEntry(uri = builder.appendPath("p$idx").build(),
                        title = "Parent $idx",
                        icon = parentIcon().takeIf { idx == 0 } // only first
                    ))
                }
            }
            val uri = breadcrumbs.last().uri
            return Model.State().withContent(
                Model.Content(
                    breadcrumbs = breadcrumbs,
                    entries = Array(dirs + files) { idx ->
                        val builder = uri.buildUpon()
                        if (idx < dirs) {
                            ListingEntry.makeFolder(uri = builder.appendEncodedPath("dir$idx")
                                .build(),
                                title = "Dir$idx",
                                description = "Directory $idx",
                                icon = folderIcon().takeIf { idx == 0 } // first one
                            )
                        } else {
                            ListingEntry.makeFile(
                                uri = builder.appendEncodedPath("file$idx").build(),
                                title = "File$idx",
                                description = "File $idx",
                                details = "${idx}0K",
                                tracks = (idx - (dirs + 1)).takeIf { it >= 0 },
                                cached = (idx > dirs + 1)
                            )
                        }
                    }.toList(),
                )
            )
        }
    }
}
