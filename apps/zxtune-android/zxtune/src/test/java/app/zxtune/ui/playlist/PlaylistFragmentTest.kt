package app.zxtune.ui.playlist

import android.content.Context
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.view.View
import android.widget.SearchView
import android.widget.TextView
import androidx.arch.core.executor.testing.InstantTaskExecutorRule
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentActivity
import androidx.fragment.app.testing.FragmentScenario
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.recyclerview.selection.MutableSelection
import androidx.recyclerview.widget.RecyclerView
import app.zxtune.R
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import app.zxtune.device.media.MediaSessionModel
import app.zxtune.playlist.ProviderClient
import app.zxtune.ui.AsyncDifferInMainThreadRule
import org.junit.After
import org.junit.Assert.assertEquals
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
@Config(shadows = [ShadowProviderClient::class, ShadowModel::class, ShadowMediaSessionModel::class])
class PlaylistFragmentTest {

    @get:Rule
    val instantTaskExecutorRule = InstantTaskExecutorRule()

    @get:Rule
    val mainThreadDiffer = AsyncDifferInMainThreadRule()

    private val client = mock<ProviderClient>()
    private val model = mock<Model>()
    private val sessionModel = mock<MediaSessionModel>()

    private val mocks
        get() = arrayOf(client, model, sessionModel)

    @Before
    fun setUp() {
        ShadowProviderClient.instance = client
        ShadowModel.instance = model
        ShadowMediaSessionModel.instance = sessionModel
        reset(*mocks)
    }

    @After
    fun tearDown() = verifyNoMoreInteractions(*mocks)

    private fun startScenario() = FragmentScenario.launchInContainer(PlaylistFragment::class.java)

    @Test
    fun `initial state`() {
        val testState = mock<LiveData<Model.State>>()
        val testPlaybackState = mock<LiveData<PlaybackStateCompat>>()
        val testMetadata = mock<LiveData<MediaMetadataCompat>>()
        model.stub {
            on { state } doReturn testState
        }
        sessionModel.stub {
            on { state } doReturn testPlaybackState
            on { metadata } doReturn testMetadata
        }
        startScenario().onFragment {
            verify(model).state
            verify(testState).observe(eq(it.viewLifecycleOwner), any())
            verify(sessionModel).state
            verify(testPlaybackState).observe(eq(it.viewLifecycleOwner), any())
            verify(sessionModel).metadata
            verify(testMetadata).observe(eq(it.viewLifecycleOwner), any())
        }.close()
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
        val testState = MutableLiveData<Model.State>()
        val testPlaybackState = MutableLiveData<PlaybackStateCompat>()
        val testMetadata = MutableLiveData<MediaMetadataCompat>()
        model.stub {
            on { state } doReturn testState
        }
        sessionModel.stub {
            on { state } doReturn testPlaybackState
            on { metadata } doReturn testMetadata
        }
        startScenario().onFragment {
            requireNotNull(it.view).run {
                val listing = findViewById<RecyclerView>(R.id.playlist_content)
                val stub = findViewById<TextView>(R.id.playlist_stub)
                testState.value = Model.State()
                assertEquals(View.GONE, listing.visibility)
                assertEquals(View.VISIBLE, stub.visibility)
                testState.value = Model.State(content)
                Robolectric.flushForegroundThreadScheduler()
                assertEquals(View.VISIBLE, listing.visibility)
                assertEquals(View.GONE, stub.visibility)
                assertEquals(content.size, listing.childCount)
            }
        }.close()
        clearInvocations(model, sessionModel)
    }

    @Test
    fun `search filtering`() {
        val content = arrayListOf(
            Entry(1, Identifier.EMPTY, "First entry", "Author1", TimeStamp.EMPTY),
            Entry(2, Identifier.EMPTY, "Second entry", "Author2", TimeStamp.EMPTY),
            Entry(3, Identifier.EMPTY, "Third entry", "second author", TimeStamp.EMPTY)
        )
        val testState = MutableLiveData(Model.State(content))
        val testPlaybackState = MutableLiveData<PlaybackStateCompat>()
        val testMetadata = MutableLiveData<MediaMetadataCompat>()
        model.stub {
            on { state } doReturn testState
            on { filter(any()) } doAnswer {
                testState.value = requireNotNull(testState.value).withFilter(it.getArgument(0))
            }
        }
        sessionModel.stub {
            on { state } doReturn testPlaybackState
            on { metadata } doReturn testMetadata
        }
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
        clearInvocations(model, sessionModel)
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

@Implements(ProviderClient::class)
class ShadowProviderClient {
    companion object {
        @JvmStatic
        @Implementation
        fun create(ctx: Context) = instance

        internal lateinit var instance: ProviderClient
    }
}

@Implements(Model.Companion::class)
class ShadowModel {
    @Implementation
    fun of(owner: Fragment): Model = instance

    companion object {
        internal lateinit var instance: Model
    }
}

@Implements(MediaSessionModel::class)
class ShadowMediaSessionModel {
    companion object {
        @JvmStatic
        @Implementation
        fun of(owner: FragmentActivity): MediaSessionModel = instance

        internal lateinit var instance: MediaSessionModel
    }
}
