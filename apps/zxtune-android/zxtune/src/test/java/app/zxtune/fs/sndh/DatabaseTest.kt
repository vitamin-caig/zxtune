package app.zxtune.fs.sndh

import androidx.core.util.Consumer
import androidx.room.Room
import androidx.test.core.app.ApplicationProvider
import app.zxtune.assertThrows
import java.io.IOException
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNull
import org.junit.Assert.assertSame
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.mock
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoInteractions
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class DatabaseTest {
    private lateinit var underTest: Database

    @Before
    fun setUp() {
        underTest = Database(
            Room.inMemoryDatabaseBuilder(
                ApplicationProvider.getApplicationContext(),
                SndhDatabaseDelegate::class.java
            ).allowMainThreadQueries().build()
        )
    }

    @After
    fun tearDown() = underTest.close()

    @Test
    fun `test runInTransaction`() {
        val error = IOException("Error")
        assertSame(error, assertThrows {
            underTest.runInTransaction {
                `test empty database`()
                underTest.addAuthor(makeAuthor(0))
                throw error
            }
        })
        `test empty database`()
    }

    @Test
    fun `test empty database`() = with(underTest) {
        val authors = mock<Consumer<Author>>()
        assertFalse(queryAuthors(authors))
        val tracks = mock<Consumer<Track>>()
        assertFalse(queryTracks(makeAuthor(0), tracks))
        val found = mock<Catalog.FoundTracksVisitor>()
        findTracks("query", found)
        assertNull(queryRandomTrack())
        verifyNoInteractions(authors, tracks, found)
    }

    @Test
    fun `test authors`() = with(underTest) {
        val authors = Array(5, ::makeAuthor)
        authors.forEach(this::addAuthor)
        withMock<Consumer<Author>> {
            assertTrue(queryAuthors(this))
            authors.forEach { verify(this).accept(it) }
        }
        cleanupAuthors()
        withMock<Consumer<Author>> {
            assertFalse(queryAuthors(this))
        }
    }

    @Test
    fun `test tracks`() = with(underTest) {
        val author1 = makeAuthor(0)
        val author2 = makeAuthor(1)
        val tracks1 = Array(5, ::makeTrack)
        val tracks2 = Array(5) { makeTrack(100 + it) }
        tracks1.forEach {
            addTrack(author1, it)
        }
        tracks2.forEach {
            addTrack(author2, it)
        }
        withMock<Consumer<Track>> {
            assertTrue(queryTracks(author1, this))
            tracks1.forEach { verify(this).accept(it) }
        }
        withMock<Consumer<Track>> {
            assertTrue(queryTracks(author2, this))
            tracks2.forEach { verify(this).accept(it) }
        }
        cleanupTracks(author1)
        withMock<Consumer<Track>> {
            assertFalse(queryTracks(author1, this))
        }
        withMock<Consumer<Track>> {
            assertTrue(queryTracks(author2, this))
            tracks2.forEach { verify(this).accept(it) }
        }
    }

    @Test
    fun `test findTracks`() = with(underTest) {
        val author1 = makeAuthor(0)
        val author2 = makeAuthor(1)
        val tracks1 = Array(3, ::makeTrack)
        val tracks2 = Array(3) { makeTrack(100 + it) }
        tracks1.forEach {
            addTrack(author1, it)
        }
        tracks2.forEach {
            addTrack(author2, it)
        }

        val found = mutableListOf<Pair<Author, Track>>()
        findTracks("Track 0") { author: Author, track: Track -> found.add(author to track) }
        assertEquals(listOf(author1 to tracks1[0]), found)
    }

    @Test
    fun `test random track`() = with(underTest) {
        assertNull(queryRandomTrack())
        val author = makeAuthor(0)
        val tracks = Array(10, ::makeTrack)
        tracks.forEach {
            addTrack(author, it)
        }
        val random = queryRandomTrack()
        assertTrue(random in tracks)
    }

    companion object {
        private fun makeAuthor(id: Int) = Author("Author $id", id)

        private fun makeTrack(id: Int) = Track(id, "Track $id", "$id KiB")

        private inline fun <reified T : Any> withMock(block: T.() -> Unit) = mock<T>().run {
            block()
            verifyNoMoreInteractions(this)
        }
    }
}
