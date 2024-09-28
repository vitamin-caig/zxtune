package app.zxtune.ui.playlist

import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.view.View
import android.widget.TextView
import androidx.appcompat.widget.SearchView
import androidx.fragment.app.testing.FragmentScenario
import androidx.recyclerview.selection.MutableSelection
import androidx.recyclerview.widget.RecyclerView
import app.zxtune.R
import app.zxtune.TestUtils.construct
import app.zxtune.TestUtils.constructedInstance
import app.zxtune.TestUtils.flushEvents
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import app.zxtune.device.media.MediaModel
import app.zxtune.ui.AsyncDifferInMainThreadRule
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.test.runTest
import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class PlaylistFragmentTest {

    @get:Rule
    val mainThreadDiffer = AsyncDifferInMainThreadRule()

    private fun startScenario() = FragmentScenario.launchInContainer(
        fragmentClass = PlaylistFragment::class.java,
        themeResId = R.style.CustomTheme,
    )

    @Test
    fun `initial state`() = runTest {
        val testState = Model.createState()
        val testStateFlow = MutableStateFlow(testState)
        val testPlaybackState = MutableStateFlow<PlaybackStateCompat?>(null)
        val testMetadata = MutableStateFlow<MediaMetadataCompat?>(null)
        val modelConstruction = construct<Model> {
            on { state } doReturn testStateFlow
            on { filter } doReturn ""
        }
        val mediaModelConstruction = construct<MediaModel> {
            on { playbackState } doReturn testPlaybackState
            on { metadata } doReturn testMetadata
        }
        modelConstruction.use {
            mediaModelConstruction.use {
                startScenario().onFragment {
                    flushEvents()
                    val model = modelConstruction.constructedInstance
                    val mediaModel = mediaModelConstruction.constructedInstance
                    verify(model).state
                    verify(mediaModel).playbackState
                    verify(mediaModel).metadata
                    verify(model).filter
                    verifyNoMoreInteractions(model, mediaModel)
                }.close()
            }
        }
    }

    @Test
    fun `with state`() = runTest {
        val content = MutableList(5) {
            Entry(
                it.toLong(),
                Identifier.parse("scheme://host/path/$it"),
                "Title $it",
                "Author $it",
                TimeStamp.fromSeconds(it + 10L)
            )
        }
        val testStateFlow = MutableStateFlow(Model.createState())
        val testPlaybackState = MutableStateFlow<PlaybackStateCompat?>(null)
        val testMetadata = MutableStateFlow<MediaMetadataCompat?>(null)
        val modelConstruction = construct<Model> {
            on { state } doReturn testStateFlow
            on { filter } doReturn ""
        }
        val mediaModelConstruction = construct<MediaModel> {
            on { playbackState } doReturn testPlaybackState
            on { metadata } doReturn testMetadata
        }
        modelConstruction.use {
            mediaModelConstruction.use {
                startScenario().onFragment {
                    flushEvents()
                    requireNotNull(it.view).run {
                        val listing = findViewById<RecyclerView>(R.id.playlist_content)
                        val stub = findViewById<TextView>(R.id.playlist_stub)
                        assertEquals(View.GONE, listing.visibility)
                        assertEquals(View.VISIBLE, stub.visibility)
                        testStateFlow.value = Model.createState().withContent(content)
                        flushEvents()
                        assertEquals(View.VISIBLE, listing.visibility)
                        assertEquals(View.GONE, stub.visibility)
                        assertEquals(content.size, listing.childCount)
                    }
                }.close()
            }
        }
    }

    @Test
    fun `search filtering`() = runTest {
        val content = arrayListOf(
            Entry(1, Identifier.EMPTY, "First entry", "Author1", TimeStamp.EMPTY),
            Entry(2, Identifier.EMPTY, "Second entry", "Author2", TimeStamp.EMPTY),
            Entry(3, Identifier.EMPTY, "Third entry", "second author", TimeStamp.EMPTY)
        )
        val testStateFlow = MutableStateFlow(Model.createState().withContent(content))
        val testPlaybackState = MutableStateFlow<PlaybackStateCompat?>(null)
        val testMetadata = MutableStateFlow<MediaMetadataCompat?>(null)
        val modelConstruction = construct<Model> {
            on { state } doReturn testStateFlow
            on { filter } doReturn testStateFlow.value.filter
            on { filter = any() } doAnswer {
                testStateFlow.value = testStateFlow.value.withFilter(it.getArgument(0))
            }
        }
        val mediaModelConstruction = construct<MediaModel> {
            on { playbackState } doReturn testPlaybackState
            on { metadata } doReturn testMetadata
        }
        modelConstruction.use {
            mediaModelConstruction.use {
                startScenario().onFragment {
                    requireNotNull(it.view).run {
                        val adapter =
                            requireNotNull(findViewById<RecyclerView>(R.id.playlist_content).adapter)
                        findViewById<SearchView>(R.id.playlist_search).let { view ->
                            flushEvents()
                            assertEquals(3, adapter.itemCount)

                            view.setQuery("second", false)
                            flushEvents()
                            assertEquals(2, adapter.itemCount)

                            view.setQuery("", false)
                            flushEvents()
                            assertEquals(3, adapter.itemCount)
                        }
                    }
                }.close()
            }
        }
    }

    @Test
    fun `selection processing`() {
        val selection = MutableSelection<Long>()
        assertEquals(null, PlaylistFragment.convertSelection(selection))
        selection.add(2)
        with(requireNotNull(PlaylistFragment.convertSelection(selection))) {
            assertEquals(1, size)
            assertEquals(2, get(0))
        }
        selection.add(5)
        with(requireNotNull(PlaylistFragment.convertSelection(selection))) {
            assertEquals(2, size)
            assertEquals(5, get(1))
        }
    }
}
