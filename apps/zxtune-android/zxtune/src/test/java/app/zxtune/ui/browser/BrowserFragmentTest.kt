package app.zxtune.ui.browser

import android.content.Context
import android.net.Uri
import android.view.ViewGroup
import android.widget.Button
import android.widget.ProgressBar
import android.widget.SearchView
import androidx.arch.core.executor.testing.InstantTaskExecutorRule
import androidx.core.view.get
import androidx.fragment.app.testing.FragmentScenario
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.recyclerview.widget.RecyclerView
import app.zxtune.R
import app.zxtune.TestUtils.construct
import app.zxtune.TestUtils.constructedInstance
import app.zxtune.ui.AsyncDifferInMainThreadRule
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.Robolectric
import org.robolectric.RobolectricTestRunner
import org.robolectric.annotation.Config
import org.robolectric.annotation.Implementation
import org.robolectric.annotation.Implements

@RunWith(RobolectricTestRunner::class)
@Config(shadows = [ShadowState::class])
class BrowserFragmentTest {

    @get:Rule
    val instantTaskExecutorRule = InstantTaskExecutorRule()

    @get:Rule
    val mainThreadDiffer = AsyncDifferInMainThreadRule()

    private val persistentState = mock<State>()

    private val mocks
        get() = arrayOf(persistentState)

    @Before
    fun setUp() {
        ShadowState.instance = persistentState
        reset(*mocks)
    }

    @After
    fun tearDown() = verifyNoMoreInteractions(*mocks)

    private fun startScenario() = FragmentScenario.launchInContainer(BrowserFragment::class.java)

    @Test
    fun `first start no data`() {
        // check basic subscriptions
        val listingState = mock<LiveData<Model.State>>()
        val operationProgress = mock<LiveData<Int?>>()
        val path = Uri.parse("")
        persistentState.stub {
            on { currentPath } doReturn path
        }
        construct<Model> {
            on { state } doReturn listingState
            on { progress } doReturn operationProgress
            on { notification } doReturn mock()
            on { browse(any<Lazy<Uri>>()) } doAnswer {
                it.getArgument<Lazy<Uri>>(0).value
                Unit
            }
        }.use { modelConstruction ->
            startScenario().onFragment {
                val model = modelConstruction.constructedInstance
                verify(model).browse(any<Lazy<Uri>>())
                verify(persistentState).currentPath
                verify(model, atLeastOnce()).state
                verify(model).progress
                verify(model).notification
                verify(model).setClient(any())
                verify(listingState).value
                // listing + breadcrumbs
                verify(listingState, times(2)).observe(eq(it.viewLifecycleOwner), any())
                verify(operationProgress).observe(eq(it.viewLifecycleOwner), any())
                //verify(model).onCleared()
            }.close()
            //verifyNoMoreInteractions(modelConstruction.constructedInstance)
        }
    }

    @Test
    fun `with content and state`() {
        val listingState = makeState(3, 2, 3)
        persistentState.stub {
            on { currentPath } doReturn listingState.uri
        }
        construct<Model> {
            on { state } doReturn MutableLiveData(listingState)
            on { progress } doReturn mock()
            on { notification } doReturn mock()
        }.use { modelConstruction ->
            lateinit var model: Model
            startScenario().onFragment {
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
                    }
                }
                Robolectric.flushForegroundThreadScheduler()
                it.requireActivity().onBackPressed()
            }.close()
            inOrder(persistentState, model) {
                verify(model).browse(Uri.EMPTY)
                verify(model).browse(listingState.breadcrumbs[0].uri)
                verify(model).browse(listingState.breadcrumbs[2].uri)
                // search
                verify(model).reload()
                verify(model).browseParent()
                // at close()
                verify(persistentState).currentViewPosition = 0
                //verify(model).onClear()
            }
            //verifyNoMoreInteractions(modelConstruction.constructedInstance)
        }
    }

    @Test
    fun `progress state`() {
        val operationProgress = MutableLiveData<Int?>()
        persistentState.stub {
            on { currentPath } doReturn Uri.EMPTY
        }
        construct<Model> {
            on { state } doReturn mock()
            on { progress } doReturn operationProgress
            on { notification } doReturn mock()
        }.use {
            startScenario().onFragment {
                clearInvocations(*mocks)
                it.view!!.findViewById<ProgressBar>(R.id.browser_loading).run {
                    operationProgress.value = null
                    assertEquals(0, progress)
                    assertEquals(false, isIndeterminate)

                    operationProgress.value = -1
                    assertEquals(0, progress)
                    assertEquals(true, isIndeterminate)

                    operationProgress.value = 0
                    assertEquals(0, progress)
                    assertEquals(false, isIndeterminate)

                    operationProgress.value = 100
                    assertEquals(100, progress)
                    assertEquals(false, isIndeterminate)
                }
            }.close()
        }
    }

    @Test
    fun `search filtering`() {
        val listingState = MutableLiveData(makeState(5, 3, 4))
        persistentState.stub {
            on { currentPath } doReturn listingState.value!!.uri
        }
        construct<Model> {
            on { state } doReturn listingState
            on { progress } doReturn mock()
            on { notification } doReturn mock()
            on { filter(any()) } doAnswer {
                listingState.value = listingState.value!!.withFilter(it.getArgument(0))
            }
        }.use { modelConstruction ->
            startScenario().onFragment {
                val model = modelConstruction.constructedInstance
                clearInvocations(model)
                it.view!!.run {
                    val adapter = findViewById<RecyclerView>(R.id.browser_content).adapter!!
                    findViewById<SearchView>(R.id.browser_search).let { view ->
                        assertEquals(7, adapter.itemCount)

                        assertTrue(view.requestFocus())
                        view.setQuery("Dir", false)
                        Robolectric.flushForegroundThreadScheduler()
                        assertEquals(3, adapter.itemCount)

                        view.setQuery("", false)
                        Robolectric.flushForegroundThreadScheduler()
                        assertEquals(7, adapter.itemCount)

                        view.setQuery("File ", false)
                        Robolectric.flushForegroundThreadScheduler()
                        assertEquals(4, adapter.itemCount)

                        view.setQuery("Model query", true)
                    }
                }
                inOrder(model) {
                    verify(model).filter("Dir")
                    verify(model).filter("")
                    verify(model).filter("File ")
                    verify(model).filter("Model query")
                    verify(model).search("Model query")
                    //verify(model).onClear()
                }
                //verifyNoMoreInteractions(model)
            }.close()
            clearInvocations(*mocks)
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
                    add(BreadcrumbsEntry(
                        uri = builder.appendPath("p$idx").build(),
                        title = "Parent $idx",
                        icon = parentIcon().takeIf { idx == 0 } // only first
                    ))
                }
            }
            val uri = breadcrumbs.last().uri
            return Model.State().withContent(
                breadcrumbs = breadcrumbs,
                entries = Array(dirs + files) { idx ->
                    val builder = uri.buildUpon()
                    if (idx < dirs) {
                        ListingEntry.makeFolder(
                            uri = builder.appendEncodedPath("dir$idx").build(),
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
                            cached = (idx > dirs + 1).takeIf { idx >= dirs }
                        )
                    }
                }.toList(),
            )
        }
    }
}

@Implements(State::class)
internal class ShadowState {
    companion object {
        internal lateinit var instance: State

        @JvmStatic
        @Implementation
        fun create(ctx: Context): State = instance
    }
}
