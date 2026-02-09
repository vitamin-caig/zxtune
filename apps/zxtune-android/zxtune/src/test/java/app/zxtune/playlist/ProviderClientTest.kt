package app.zxtune.playlist

import android.content.ContentResolver
import android.database.ContentObserver
import android.database.MatrixCursor
import android.os.CancellationSignal
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.collectIndexed
import kotlinx.coroutines.launch
import kotlinx.coroutines.test.StandardTestDispatcher
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.anyOrNull
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.eq
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.reset
import org.mockito.kotlin.stub
import org.mockito.kotlin.times
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class ProviderClientTest {

    private val playlist = Playlist.DEFAULT_ID
    private val dispatcher = StandardTestDispatcher()
    private val resolver = mock<ContentResolver>()

    @Before
    fun setUp() = reset(resolver)

    @After
    fun tearDown() = verifyNoMoreInteractions(resolver)

    @Test
    fun observeContent() = runTest {
        lateinit var observer: ContentObserver
        resolver.stub {
            on { registerContentObserver(any(), any(), any()) } doAnswer {
                observer = it.getArgument(2)
            }
            on {
                query(
                    any(), anyOrNull(), anyOrNull(), anyOrNull(), anyOrNull(), any()
                )
            } doReturn mock()
        }
        launch(SupervisorJob()) {
            ProviderClient(resolver, playlist,dispatcher).observeContent().collectIndexed {
                index, _ ->
                if (index == 9) {
                    cancel()
                } else {
                    observer.onChange(false)
                }
            }
        }.join()

        val uri = Query.localPlaylistUri(playlist)
        inOrder(resolver) {
            verify(resolver).registerContentObserver(uri, true, observer)
            verify(resolver, times(10)).query(
                eq(uri), eq(null), eq(null), eq(null), eq(null), any()
            )
            verify(resolver).unregisterContentObserver(observer)
        }
    }

    @Test
    fun query() = runTest {
        val columns = IO.TrackColumns.entries.map { it.name }.toTypedArray()
        val content = MatrixCursor(columns, 1).apply {
            addRow(arrayOf<Any>(123L, "scheme://host/path#fragment", "title", "author", 123456L))
        }
        lateinit var signal: CancellationSignal
        resolver.stub {
            on {
                query(
                    any(), anyOrNull(), anyOrNull(), anyOrNull(), anyOrNull(), any()
                )
            } doAnswer {
                signal = it.getArgument(5)
                content
            }
        }
        requireNotNull(ProviderClient(resolver, playlist, dispatcher).queryContent()).run {
            assertEquals(1, size)
            get(0).run {
                assertEquals(Track.Id(123), id)
                assertEquals(
                    Identifier.parse("scheme://host/path#fragment"), meta.location
                )
                assertEquals("author", meta.author)
                assertEquals("title", meta.title)
                assertEquals(TimeStamp.fromMilliseconds(123456), meta.duration)
            }
        }
        verify(resolver).query(Query.localPlaylistUri(playlist), null, null, null, null, signal)
    }

    //TODO: add another tests
}
