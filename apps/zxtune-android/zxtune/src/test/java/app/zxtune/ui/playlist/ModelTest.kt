package app.zxtune.ui.playlist

import app.zxtune.TestUtils.flushEvents
import app.zxtune.TestUtils.mockCollectorOf
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import app.zxtune.playlist.PlaylistContent
import app.zxtune.playlist.ProviderClient
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
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

    private val dispatcher = StandardTestDispatcher()

    @Before
    fun setUp() = reset(client)

    @After
    fun tearDown() = verifyNoMoreInteractions(client)

    @Test
    fun `no state retrieval`() {
        Model(mock(), client, dispatcher)
        verify(client).observeContent()
    }

    @Test
    fun `basic workflow`() = runTest(dispatcher) {
        val list1 = PlaylistContent(1).apply {
            add(mock<Entry>())
        }
        val list2 = PlaylistContent(2).apply {
            add(mock<Entry>())
            add(mock<Entry>())
        }
        val contentFlow = flow {
            emit(list2)
            emit(list1)
        }
        client.stub {
            on { observeContent() } doReturn contentFlow
        }
        val cb = mockCollectorOf(Model(mock(), client, dispatcher).state)
        inOrder(cb, client) {
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
            assertNull(entries as? MutableList<Entry>)
            assertEquals(0, entries.size)
            assertEquals("", filter)
        }
        val entry1 = Entry(1, Identifier.EMPTY, "First entry", "Author1", TimeStamp.EMPTY)
        val entry2 = Entry(2, Identifier.EMPTY, "Second entry", "Author2", TimeStamp.EMPTY)
        val entry3 = Entry(3, Identifier.EMPTY, "Third entry", "second author", TimeStamp.EMPTY)
        val entry4 = Entry(
            4, Identifier.parse("schema://host/VisiblePath"), "title", "aut", TimeStamp.EMPTY
        )
        val filled2 = initial.withContent(arrayListOf(entry1, entry2)).apply {
            assertNotNull(entries as? MutableList<Entry>)
            assertEquals("", filter)
            assertEquals(arrayListOf(entry1, entry2), entries)
        }
        assertEquals(initial, filled2.withContent(arrayListOf()))
        assertEquals(filled2, filled2.withFilter(" "))

        val filtered2 = filled2.withFilter("second").apply {
            assertNull(entries as? MutableList<Entry>)
            assertEquals("second", filter)
            assertEquals(arrayListOf(entry2), entries)
        }
        filtered2.withContent(arrayListOf()).apply {
            assertNull(entries as? MutableList<Entry>)
            assertEquals("second", filter)
            assertEquals(0, entries.size)
        }

        val filtered3 = filtered2.withContent(arrayListOf(entry3, entry1, entry2, entry4)).apply {
            assertNull(entries as? MutableList<Entry>)
            assertEquals("second", filter)
            assertEquals(arrayListOf(entry3, entry2), entries)
        }

        /*val filled3 = */filtered3.withFilter(" ").apply {
            assertNotNull(entries as? MutableList<Entry>)
            assertEquals("", filter)
            assertEquals(arrayListOf(entry3, entry1, entry2, entry4), entries)
        }
        /*val filled4 = */filtered3.withFilter("visible").apply {
            assertNull(entries as? MutableList<Entry>)
            assertEquals("visible", filter)
            assertEquals(arrayListOf(entry4), entries)
        }
    }
}
