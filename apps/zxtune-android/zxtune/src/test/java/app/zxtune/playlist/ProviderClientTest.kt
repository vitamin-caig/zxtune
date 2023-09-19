import android.content.ContentResolver
import android.database.ContentObserver
import android.database.MatrixCursor
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import app.zxtune.playlist.Database
import app.zxtune.playlist.PlaylistQuery
import app.zxtune.playlist.ProviderClient
import app.zxtune.ui.MainDispatcherRule
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.collectIndexed
import kotlinx.coroutines.launch
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Rule
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.anyOrNull
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.reset
import org.mockito.kotlin.stub
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class ProviderClientTest {

    private val dispatcher = StandardTestDispatcher()

    @get:Rule
    val mainDispatcher = MainDispatcherRule(dispatcher)

    private val resolver = mock<ContentResolver>()

    @Before
    fun setUp() = reset(resolver)

    @After
    fun tearDown() = verifyNoMoreInteractions(resolver)

    @Test
    fun observeChanges() = runTest {
        lateinit var observer: ContentObserver
        resolver.stub {
            on { registerContentObserver(any(), any(), any()) } doAnswer {
                observer = it.getArgument(2)
            }
        }
        launch(SupervisorJob()) {
            ProviderClient(resolver).observeChanges().collectIndexed { index, _ ->
                if (index == 10) {
                    cancel()
                } else {
                    observer.onChange(false)
                }
            }
        }.join()

        inOrder(resolver) {
            verify(resolver).registerContentObserver(PlaylistQuery.ALL, true, observer)
            verify(resolver).unregisterContentObserver(observer)
        }
    }

    @Test
    fun query() {
        // _id, pos, location, author, title, duration, properties
        val columns = Database.Tables.Playlist.Fields.values().map { it.toString() }.toTypedArray()
        val content = MatrixCursor(columns, 1).apply {
            addRow(
                arrayOf(
                    123, 0, "scheme://host/path#fragment", "author", "title", 123456L, byteArrayOf()
                )
            )
        }
        resolver.stub {
            on { query(any(), anyOrNull(), anyOrNull(), anyOrNull(), anyOrNull()) } doReturn content
        }
        requireNotNull(ProviderClient(resolver).query(null)).run {
            assertEquals(1, size)
            get(0).run {
                assertEquals(123, id)
                assertEquals(
                    Identifier.parse("scheme://host/path#fragment"), location
                )
                assertEquals("author", author)
                assertEquals("title", title)
                assertEquals(TimeStamp.fromMilliseconds(123456), duration)
            }
        }
        verify(resolver).query(PlaylistQuery.ALL, null, null, null, null)
    }

    //TODO: add another tests
}
