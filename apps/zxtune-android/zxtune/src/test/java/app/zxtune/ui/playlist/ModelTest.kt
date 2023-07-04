package app.zxtune.ui.playlist

import android.database.MatrixCursor
import androidx.arch.core.executor.testing.InstantTaskExecutorRule
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import app.zxtune.playlist.Database
import app.zxtune.playlist.ProviderClient
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import java.util.concurrent.ExecutorService

private fun assertEquals(ref: State, test: State) {
    assertEquals(ref.entries, test.entries)
    assertEquals(ref.filter, test.filter)
}

@RunWith(RobolectricTestRunner::class)
class ModelTest {

    @get:Rule
    val instantTaskExecutorRule = InstantTaskExecutorRule()

    private val client = mock<ProviderClient>()
    private val async = mock<ExecutorService> {
        on { execute(any()) } doAnswer {
            it.getArgument<Runnable>(0).run()
        }
    }

    @Before
    fun setUp() {
        reset(client)
        clearInvocations(async)
    }

    @After
    fun tearDown() {
        verifyNoMoreInteractions(client, async)
    }

    @Test
    fun `no state retrieval`() {
        lateinit var observer: ProviderClient.ChangesObserver
        client.stub {
            on { registerObserver(any()) } doAnswer {
                observer = it.getArgument(0)
            }
        }
        val underTest = Model(mock(), client, async)
        observer.onChange()
        underTest.onCleared()

        inOrder(client) {
            verify(client).registerObserver(any())
            verify(client).unregisterObserver()
        }
    }

    @Test
    fun `basic workflow`() {
        // _id, pos, location, author, title, duration, properties
        val columns = Database.Tables.Playlist.Fields.values().map { it.toString() }.toTypedArray()
        val emptyList = MatrixCursor(columns)
        val nonEmptyList = MatrixCursor(columns, 1).apply {
            addRow(
                arrayOf(
                    123,
                    0,
                    "scheme://host/path#fragment",
                    "author",
                    "title",
                    123456L,
                    byteArrayOf()
                )
            )
        }
        lateinit var observer: ProviderClient.ChangesObserver
        client.stub {
            on { registerObserver(any()) } doAnswer {
                observer = it.getArgument(0)
            }
            on { query(null) }.doReturn(emptyList, nonEmptyList)
        }
        with(Model(mock(), client, async)) {
            assertEquals(0, requireNotNull(state.value).entries.size)
            observer.onChange()
            requireNotNull(state.value).entries.run {
                assertEquals(1, size)
                get(0).run {
                    assertEquals(123, id)
                    assertEquals(Identifier.parse("scheme://host/path#fragment"), location)
                    assertEquals("author", author)
                    assertEquals("title", title)
                    assertEquals(TimeStamp.fromMilliseconds(123456), duration)
                }
            }
            onCleared()
        }

        inOrder(client, async) {
            verify(client).registerObserver(any())
            verify(async).execute(any())
            verify(client).query(null)
            verify(async).execute(any())
            verify(client).query(null)
            verify(client).unregisterObserver()
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
        val entry4 =
            Entry(4, Identifier.parse("schema://host/VisiblePath"), "", "aut", TimeStamp.EMPTY)
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
