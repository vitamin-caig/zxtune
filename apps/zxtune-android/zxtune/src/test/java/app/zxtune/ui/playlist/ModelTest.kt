package app.zxtune.ui.playlist

import app.zxtune.TestUtils.flushEvents
import app.zxtune.TestUtils.mockCollectorOf
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import app.zxtune.playlist.AggregatingProviderClient
import app.zxtune.playlist.Playlist
import app.zxtune.playlist.PlaylistContent
import app.zxtune.playlist.ProviderClient
import app.zxtune.playlist.Track
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.argThat
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.reset
import org.mockito.kotlin.stub
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner

private fun assertEquals(ref: State, test: State) {
    assertEquals(ref.entries, test.entries)
    assertEquals(ref.filter, test.filter)
}

@RunWith(RobolectricTestRunner::class)
class ModelTest {

    private val client = mock<ProviderClient>()
    private val aggregatingClient = mock<AggregatingProviderClient>() {
        on { getPlaylist(any()) } doReturn client
    }

    private val dispatcher = StandardTestDispatcher()

    @Before
    fun setUp() = reset(client)

    @After
    fun tearDown() = verifyNoMoreInteractions(aggregatingClient, client)

    @Test
    fun `no state retrieval`() {
        Model(mock(), aggregatingClient, dispatcher)
        verify(aggregatingClient).getPlaylist(Playlist.DEFAULT_ID)
        verify(client).observeContent()
    }

    @Test
    fun `basic workflow`() = runTest(dispatcher) {
        val list1 = PlaylistContent(1).apply {
            add(mock<Track>())
        }
        val list2 = PlaylistContent(2).apply {
            add(mock<Track>())
            add(mock<Track>())
        }
        val contentFlow = flow {
            emit(list2)
            emit(list1)
        }
        client.stub {
            on { observeContent() } doReturn contentFlow
        }
        val cb = mockCollectorOf(Model(mock(), aggregatingClient, dispatcher).listing)
        inOrder(cb, client) {
            verify(aggregatingClient).getPlaylist(Playlist.DEFAULT_ID)
            flushEvents()
            verify(client).observeContent()

            // initial state + empty response
            verify(cb).invoke(argThat { entries.isEmpty() })

            verify(cb).invoke(argThat { entries == list2 })
            verify(cb).invoke(argThat { entries == list1 })
        }
        verifyNoMoreInteractions(cb)
    }

    @Test
    fun `state logic`() {
        val initial = Model.createState().apply {
            assertNull(entries as? MutableList<Track>)
            assertEquals(0, entries.size)
            assertEquals("", filter)
        }
        val makeTrack = { id: Long, location: Identifier, title: String, author: String ->
            Track(Track.Id(id), Track.Metadata(location, title, author, TimeStamp.EMPTY))
        }
        val track1 = makeTrack(1, Identifier.EMPTY, "First", "Author1")
        val track2 = makeTrack(2, Identifier.EMPTY, "Second", "Author2")
        val track3 = makeTrack(3, Identifier.EMPTY, "Third", "second author")
        val track4 = makeTrack(4, Identifier.parse("schema://host/VisiblePath"), "title", "aut")
        val filled2 = initial.withContent(arrayListOf(track1, track2)).apply {
            assertNotNull(entries as? MutableList<Track>)
            assertEquals("", filter)
            assertEquals(arrayListOf(track1, track2), entries)
        }
        assertEquals(initial, filled2.withContent(arrayListOf()))
        assertEquals(filled2, filled2.withFilter(" "))

        val filtered2 = filled2.withFilter("second").apply {
            assertNull(entries as? MutableList<Track>)
            assertEquals("second", filter)
            assertEquals(arrayListOf(track2), entries)
        }
        filtered2.withContent(arrayListOf()).apply {
            assertNull(entries as? MutableList<Track>)
            assertEquals("second", filter)
            assertEquals(0, entries.size)
        }

        val filtered3 = filtered2.withContent(arrayListOf(track3, track1, track2, track4)).apply {
            assertNull(entries as? MutableList<Track>)
            assertEquals("second", filter)
            assertEquals(arrayListOf(track3, track2), entries)
        }

        /*val filled3 = */filtered3.withFilter(" ").apply {
            assertNotNull(entries as? MutableList<Track>)
            assertEquals("", filter)
            assertEquals(arrayListOf(track3, track1, track2, track4), entries)
        }
        /*val filled4 = */filtered3.withFilter("visible").apply {
            assertNull(entries as? MutableList<Track>)
            assertEquals("visible", filter)
            assertEquals(arrayListOf(track4), entries)
        }
    }
}
