package app.zxtune.ui.playlist

import android.database.MatrixCursor
import androidx.arch.core.executor.testing.InstantTaskExecutorRule
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import app.zxtune.playlist.Database
import app.zxtune.playlist.ProviderClient
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import java.util.concurrent.ExecutorService

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
            assertEquals(0, requireNotNull(items.value).size)
            observer.onChange()
            requireNotNull(items.value).run {
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
}
