package app.zxtune.fs.sndh

import android.net.Uri
import app.zxtune.fs.api.Cdn
import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class RemoteCatalogTest {
    private lateinit var catalog: RemoteCatalog

    @Before
    fun setUp() {
        catalog = RemoteCatalog(MultisourceHttpProvider(HttpProviderFactory.createTestProvider()))
    }

    @Test
    fun `authors`() {
        val authors = ArrayList<Author>()
        catalog.queryAuthors({ authors.add(it) })

        assertTrue(authors.size > 100)
        val author = authors.find { it.name == "505" }
        assertNotNull(author)
        assertTrue(author!!.files > 0)
    }

    @Test
    fun `author tracks`() {
        val tracks = ArrayList<Track>()
        catalog.queryTracks(Author("505"), { tracks.add(it) })

        assertTrue(tracks.size > 100)
        val track = tracks.find { it.id == 2780 }
        assertNotNull(track)
        assertEquals("052", track!!.title)
        assertTrue(track.sizeText, track.sizeText.endsWith("KiB"))
    }

    @Test
    fun `get track uris`() {
        val uris = RemoteCatalog.getTrackUris(12345)
        assertEquals(Cdn.sndh(12345), uris.next())
        assertEquals(Uri.parse("https://sndh.atari.org/dl.php?ID=12345"), uris.next())
        assertFalse(uris.hasNext())
    }

    @Test
    fun `find tracks`() {
        val found = ArrayList<Pair<Author, Track>>()
        catalog.findTracks("relix") { author, track -> found.add(author to track) }

        val (author, track) = found.first { it.second.id == 4302 }
        assertEquals("505", author.name)
        assertEquals("Relix", track.title)
    }

    @Test
    fun `random track`() {
        val track = catalog.findRandomTrack()

        assertNotNull(track)
        assertTrue(track!!.id > 0)
        assertTrue(track.title.isNotBlank())
        assertTrue(track.sizeText.endsWith("bytes"))
    }
}
