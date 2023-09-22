package app.zxtune.ui.playlist

import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.view.View
import android.widget.TextView
import androidx.appcompat.widget.SearchView
import androidx.arch.core.executor.testing.InstantTaskExecutorRule
import androidx.fragment.app.testing.FragmentScenario
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.recyclerview.selection.MutableSelection
import androidx.recyclerview.widget.RecyclerView
import app.zxtune.R
import app.zxtune.TestUtils.construct
import app.zxtune.TestUtils.constructedInstance
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import app.zxtune.device.media.MediaModel
import app.zxtune.ui.AsyncDifferInMainThreadRule
import org.junit.Assert.assertEquals
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.Robolectric
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class PlaylistFragmentTest {

    @get:Rule
    val instantTaskExecutorRule = InstantTaskExecutorRule()

    @get:Rule
    val mainThreadDiffer = AsyncDifferInMainThreadRule()

    private fun startScenario() = FragmentScenario.launchInContainer(
        fragmentClass = PlaylistFragment::class.java,
        themeResId = R.style.CustomTheme,
    )

    @Test
    fun `initial state`() {
        val testState = mock<LiveData<State>>()
        val testPlaybackState = mock<LiveData<PlaybackStateCompat?>>()
        val testMetadata = mock<LiveData<MediaMetadataCompat?>>()
        val modelConstruction = construct<Model> {
            on { state } doReturn testState
        }
        val mediaModelConstruction = construct<MediaModel> {
            on { playbackState } doReturn testPlaybackState
            on { metadata } doReturn testMetadata
        }
        modelConstruction.use {
            mediaModelConstruction.use {
                startScenario().onFragment {
                    val model = modelConstruction.constructedInstance
                    val mediaModel = mediaModelConstruction.constructedInstance
                    verify(model, times(2)).state
                    verify(testState).observe(eq(it.viewLifecycleOwner), any())
                    verify(mediaModel).playbackState
                    verify(testPlaybackState).observe(eq(it.viewLifecycleOwner), any())
                    verify(mediaModel).metadata
                    verify(testMetadata).observe(eq(it.viewLifecycleOwner), any())
                    verify(model).filter("")
                    verifyNoMoreInteractions(model, mediaModel)
                }.close()
                verifyNoMoreInteractions(testPlaybackState, testMetadata)
            }
        }
    }

    @Test
    fun `with state`() {
        val content = MutableList(5) {
            Entry(
                it.toLong(),
                Identifier.parse("scheme://host/path/$it"),
                "Title $it",
                "Author $it",
                TimeStamp.fromSeconds(it + 10L)
            )
        }
        val testState = MutableLiveData<State>()
        val testPlaybackState = MutableLiveData<PlaybackStateCompat>()
        val testMetadata = MutableLiveData<MediaMetadataCompat>()
        val modelConstruction = construct<Model> {
            on { state } doReturn testState
        }
        val mediaModelConstruction = construct<MediaModel> {
            on { playbackState } doReturn testPlaybackState
            on { metadata } doReturn testMetadata
        }
        modelConstruction.use {
            mediaModelConstruction.use {
                startScenario().onFragment {
                    requireNotNull(it.view).run {
                        val listing = findViewById<RecyclerView>(R.id.playlist_content)
                        val stub = findViewById<TextView>(R.id.playlist_stub)
                        testState.value = Model.createState()
                        assertEquals(View.GONE, listing.visibility)
                        assertEquals(View.VISIBLE, stub.visibility)
                        testState.value = Model.createState().withContent(content)
                        Robolectric.flushForegroundThreadScheduler()
                        assertEquals(View.VISIBLE, listing.visibility)
                        assertEquals(View.GONE, stub.visibility)
                        assertEquals(content.size, listing.childCount)
                    }
                }.close()
            }
        }
    }

    @Test
    fun `search filtering`() {
        val content = arrayListOf(
            Entry(1, Identifier.EMPTY, "First entry", "Author1", TimeStamp.EMPTY),
            Entry(2, Identifier.EMPTY, "Second entry", "Author2", TimeStamp.EMPTY),
            Entry(3, Identifier.EMPTY, "Third entry", "second author", TimeStamp.EMPTY)
        )
        val testState = MutableLiveData(Model.createState().withContent(content))
        val testPlaybackState = MutableLiveData<PlaybackStateCompat>()
        val testMetadata = MutableLiveData<MediaMetadataCompat>()
        val modelConstruction = construct<Model> {
            on { state } doReturn testState
            on { filter(any()) } doAnswer {
                testState.value = requireNotNull(testState.value).withFilter(it.getArgument(0))
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
                            Robolectric.flushForegroundThreadScheduler()
                            assertEquals(3, adapter.itemCount)

                            view.setQuery("second", false)
                            Robolectric.flushForegroundThreadScheduler()
                            assertEquals(2, adapter.itemCount)

                            view.setQuery("", false)
                            Robolectric.flushForegroundThreadScheduler()
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
