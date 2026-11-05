package app.zxtune.fs.sndh

import androidx.core.util.Consumer
import app.zxtune.assertThrows
import app.zxtune.fs.dbhelpers.Timestamps
import app.zxtune.fs.dbhelpers.Utils
import java.io.IOException
import org.junit.Assert.assertEquals
import org.junit.Assert.assertSame
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.eq
import org.mockito.kotlin.mock
import org.mockito.kotlin.never
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoInteractions
import org.mockito.kotlin.verifyNoMoreInteractions
import org.mockito.kotlin.whenever
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class CachingCatalogTest {
    private val lifetime = mock<Timestamps.Lifetime>()
    private val database = mock<Database> {
        on { runInTransaction(any()) } doAnswer {
            it.getArgument<Utils.ThrowingRunnable>(0).run()
        }
        on { getLifetime(any(), any()) } doReturn lifetime
    }
    private val remote = mock<RemoteCatalog>()
    private val authorsVisitor = mock<Consumer<Author>>()
    private val tracksVisitor = mock<Consumer<Track>>()
    private val foundTracksVisitor = mock<Catalog.FoundTracksVisitor>()
    private val underTest = CachingCatalog(remote, database)
    private val error = IOException()

    @Test
    fun `test authors from fresh cache`() {
        whenever(lifetime.isExpired) doReturn false
        underTest.queryAuthors(authorsVisitor)

        verify(database).getLifetime(eq("authors"), any())
        verify(lifetime).isExpired
        verify(database).queryAuthors(authorsVisitor)
        verifyNoMoreInteractions(database, lifetime)
        verifyNoInteractions(remote, authorsVisitor)
    }

    @Test
    fun `test authors cache update`() {
        val authors = Array(5, ::makeAuthor)
        whenever(lifetime.isExpired) doReturn true
        whenever(database.queryAuthors(authorsVisitor)) doReturn true
        whenever(remote.queryAuthors(any())) doAnswer {
            val visitor = it.getArgument<Consumer<Author>>(0)
            authors.forEach(visitor::accept)
        }
        underTest.queryAuthors(authorsVisitor)

        verify(database).getLifetime(eq("authors"), any())
        verify(lifetime).isExpired
        verify(database).runInTransaction(any())
        verify(database).cleanupAuthors()
        authors.forEach {
            verify(database).addAuthor(it)
        }
        verify(remote).queryAuthors(any())
        verify(lifetime).update()
        verify(database).queryAuthors(authorsVisitor)
        verifyNoMoreInteractions(database, lifetime, remote)
    }

    @Test
    fun `test tracks from fresh cache`() {
        val author = makeAuthor(0)
        whenever(lifetime.isExpired) doReturn false
        underTest.queryTracks(author, tracksVisitor)

        verify(database).getLifetime(eq("tracks:${author.name}"), any())
        verify(lifetime).isExpired
        verify(database).queryTracks(author, tracksVisitor)
        verifyNoMoreInteractions(database, lifetime)
        verifyNoInteractions(remote, tracksVisitor)
    }

    @Test
    fun `test tracks cache update`() {
        val author = makeAuthor(0)
        val tracks = Array(5, ::makeTrack)
        whenever(lifetime.isExpired) doReturn true
        whenever(database.queryTracks(author, tracksVisitor)) doReturn true
        whenever(remote.queryTracks(eq(author), any())) doAnswer {
            val visitor = it.getArgument<Consumer<Track>>(1)
            tracks.forEach(visitor::accept)
        }
        underTest.queryTracks(author, tracksVisitor)

        verify(database).getLifetime(eq("tracks:${author.name}"), any())
        verify(lifetime).isExpired
        verify(database).runInTransaction(any())
        verify(database).cleanupTracks(author)
        tracks.forEach {
            verify(database).addTrack(author, it)
        }
        verify(remote).queryTracks(eq(author), any())
        verify(lifetime).update()
        verify(database).queryTracks(author, tracksVisitor)
        verifyNoMoreInteractions(database, lifetime, remote)
    }

    @Test
    fun `test tracks update failure with cache`() {
        val author = makeAuthor(0)
        whenever(lifetime.isExpired) doReturn true
        whenever(database.queryTracks(author, tracksVisitor)) doReturn true
        doAnswer { throw error }.whenever(remote).queryTracks(eq(author), any())
        underTest.queryTracks(author, tracksVisitor)

        verify(database).getLifetime(eq("tracks:${author.name}"), any())
        verify(lifetime).isExpired
        verify(database).runInTransaction(any())
        verify(database).cleanupTracks(author)
        verify(remote).queryTracks(eq(author), any())
        verify(lifetime, never()).update()
        verify(database).queryTracks(author, tracksVisitor)
        verifyNoMoreInteractions(database, lifetime, remote)
    }

    @Test
    fun `test tracks update failure without cache`() {
        val author = makeAuthor(0)
        whenever(lifetime.isExpired) doReturn true
        whenever(database.queryTracks(author, tracksVisitor)) doReturn false
        doAnswer { throw error }.whenever(remote).queryTracks(eq(author), any())

        assertSame(error, assertThrows {
            underTest.queryTracks(author, tracksVisitor)
        })
        verify(lifetime, never()).update()
    }

    @Test
    fun `test findTracks online`() {
        val query = "query"
        whenever(remote.searchSupported()) doReturn true
        underTest.findTracks(query, foundTracksVisitor)

        verify(remote).searchSupported()
        verify(remote).findTracks(query, foundTracksVisitor)
        verifyNoMoreInteractions(remote)
        verifyNoInteractions(database)
    }

    @Test
    fun `test findTracks offline`() {
        val query = "query"
        whenever(remote.searchSupported()) doReturn false
        underTest.findTracks(query, foundTracksVisitor)

        verify(remote).searchSupported()
        verify(database).findTracks(query, foundTracksVisitor)
        verifyNoMoreInteractions(remote, database)
    }

    @Test
    fun `test random track online`() {
        val track = makeTrack(0)
        whenever(remote.findRandomTrack()) doReturn track

        assertEquals(track, underTest.findRandomTrack())

        verify(remote).findRandomTrack()
        verifyNoInteractions(database)
    }

    @Test
    fun `test random track fallback on error`() {
        val track = makeTrack(0)
        doAnswer { throw error }.whenever(remote).findRandomTrack()
        whenever(database.queryRandomTrack()) doReturn track

        assertEquals(track, underTest.findRandomTrack())

        verify(remote).findRandomTrack()
        verify(database).queryRandomTrack()
        verifyNoMoreInteractions(remote, database)
    }

    @Test
    fun `test random track fallback on empty remote result`() {
        val track = makeTrack(0)
        whenever(remote.findRandomTrack()) doReturn null
        whenever(database.queryRandomTrack()) doReturn track

        assertEquals(track, underTest.findRandomTrack())

        verify(remote).findRandomTrack()
        verify(database).queryRandomTrack()
        verifyNoMoreInteractions(remote, database)
    }

    companion object {
        private fun makeAuthor(id: Int) = Author("Author $id", id)

        private fun makeTrack(id: Int) = Track(id, "Track $id", "$id KiB")
    }
}
