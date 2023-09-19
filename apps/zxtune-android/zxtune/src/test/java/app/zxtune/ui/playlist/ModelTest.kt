package app.zxtune.ui.playlist

import androidx.lifecycle.viewModelScope
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import app.zxtune.playlist.ProviderClient
import app.zxtune.ui.MainDispatcherRule
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.MutableSharedFlow
import kotlinx.coroutines.flow.collectIndexed
import kotlinx.coroutines.launch
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.advanceUntilIdle
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner

private fun assertEquals(ref: State, test: State) {
    assertEquals(ref.entries, test.entries)
    assertEquals(ref.filter, test.filter)
}

@OptIn(ExperimentalCoroutinesApi::class)
@RunWith(RobolectricTestRunner::class)
class ModelTest {

    private val client = mock<ProviderClient>()

    private val dispatcher = StandardTestDispatcher()

    @get:Rule
    val mainDispatcher = MainDispatcherRule(dispatcher)

    @Before
    fun setUp() = reset(client)

    @After
    fun tearDown() = verifyNoMoreInteractions(client)

    @Test
    fun `no state retrieval`() {
        Model(mock(), client, dispatcher, dispatcher)
        verify(client).observeChanges()
    }

    @Test
    fun `basic workflow`() = runTest {
        val emptyList = arrayListOf<Entry>()
        val nonEmptyList = arrayListOf(mock<Entry>())
        val updatesFlow = MutableSharedFlow<Unit>()
        client.stub {
            on { observeChanges() } doReturn updatesFlow
            on { query(null) }.doReturn(nonEmptyList, emptyList)
        }
        with(Model(mock(), client, dispatcher, dispatcher)) {
            val job = viewModelScope.launch {
                state.collectIndexed { idx, state ->
                    when (idx) {
                        // initial value in stateIn, runningFold and initial filter
                        0, 1 -> assertEquals(0, state.entries.size)
                        2 -> state.entries.run {
                            assertEquals(0, size)
                            updatesFlow.emit(Unit) // initial _stateFlow
                        }
                        // nonempty response
                        3 -> state.entries.run {
                            assertEquals(nonEmptyList, this)
                            updatesFlow.emit(Unit)
                        }
                        // empty response
                        4 -> state.entries.run {
                            assertEquals(0, size)
                            cancel()
                        }

                        else -> fail("Unexpected!")
                    }
                }
            }
            job.join()
            advanceUntilIdle()
        }

        inOrder(client) {
            verify(client).observeChanges()
            verify(client, times(2)).query(null)
        }
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
