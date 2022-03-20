package app.zxtune.fs.amp

import app.zxtune.BuildConfig
import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class RemoteCatalogTest {

    private lateinit var catalog: RemoteCatalog

    @Before
    fun setUp() = with(HttpProviderFactory.createTestProvider()) {
        catalog = RemoteCatalog(MultisourceHttpProvider(this))
    }

    @Test
    fun `test groups`() {
        val checkpoints = arrayOf(
            Group(6018, "0-Dezign"),
            Group(6013, "ABSENCE"),  // multiline
            Group(677, "Anexia"),  // utf-8
            Group(9016, "Åltalános Védelmi Hiba (AHV)")
        )
        val result = ArrayList<Group>().apply {
            catalog.queryGroups { add(it) }
        }
        check(8825, checkpoints, result)
    }

    @Test
    fun `test authors by handle`() {
        val checkpoints = arrayOf( // first
            Author(9913, "00d", "Petteri Kuittinen"),  // pivot
            Author(10, "4-Mat", "Matthew Simmonds"),  // n/a in real name -> empty
            Author(15226, "808-Style", ""),  // utf-8
            Author(13302, "8bitbubsy", "Olav Sørensen")
        )
        with(AuthorsChecker()) {
            catalog.queryAuthors("0-9", this)
            check(46, checkpoints)
        }
    }

    @Test
    fun `test authors by country`() {
        val checkpoints = arrayOf( // first
            Author(13606, "Agent Orange", "Oleg Sharonov"),  // next page
            Author(2142, "Doublestar", "Victor Shimyakov"),  // last one
            Author(8482, "Zyz", "")
        )
        with(AuthorsChecker()) {
            catalog.queryAuthors(Country(44, "Russia"), this)
            check(200, checkpoints)
        }
    }

    @Test
    fun `test authors by group`() {
        val checkpoints = arrayOf( // first
            Author(14354, "BeRo", "Benjamin Rosseaux"),  // last
            Author(8120, "Wayfinder", "Sebastian Grillmaier")
        )
        with(AuthorsChecker()) {
            catalog.queryAuthors(Group(1770, "Farbrausch"), this)
            check(11, checkpoints)
        }
    }

    private class AuthorsChecker : Catalog.AuthorsVisitor {

        private val result = ArrayList<Author>()

        override fun accept(obj: Author) {
            result.add(obj)
        }

        fun check(size: Int, checkpoints: Array<Author>) {
            check(size, checkpoints, result)
        }
    }

    @Test
    fun `test authors tracks`() {
        val checkpoints = arrayOf( // first
            Track(15892, "egometriosporasie", 186),  // instead unavailable
            Track(15934, "un peu + a l'ouest", 309),  // last
            Track(15804, "yapleindmondalagas", 190)
        )
        val result = ArrayList<Track>().apply {
            catalog.queryTracks(Author(2085, "doh", "Nicolas Dessesart")) { add(it) }
        }
        check(226 - 59, checkpoints, result)
    }

    @Test
    fun `test search`() {
        val tracksCheckpoints = arrayOf( // first
            Track(121325, "\"bzzzt-chip 4\"", 7),  // last
            Track(22313, "zzzzzzzzz", 50)
        )
        val authorsCheckpoints = arrayOf( // first
            // for multiple authors first is taken and merged into previous
            Author(8462, "Zuo", ""),
            Author(8071, "Voyce", ""),  // last
            Author(1669, "Daze", "")
        )
        val tracks = ArrayList<Track>()
        val authors = ArrayList<Author>()
        catalog.findTracks("zzz") { author, track ->
            tracks.add(track)
            // simple uniq
            if (authors.isEmpty() || authors[authors.size - 1].id != author.id) {
                authors.add(author)
            }
        }
        check(52, tracksCheckpoints, tracks)
        check(42, authorsCheckpoints, authors)
    }

    @Test
    fun `test getTrackUris`() = with(RemoteCatalog.getTrackUris(12345)) {
        assertEquals(2L, size.toLong())
        assertEquals("${BuildConfig.CDN_ROOT}/download/amp/ids/12345", get(0).toString())
        assertEquals("https://amp.dascene.net/downmod.php?index=12345", get(1).toString())
    }
}

private fun <T> check(minSize: Int, expected: Array<T>, actual: ArrayList<T>) {
    assertTrue(actual.size >= minSize)
    for (obj in expected) {
        assertTrue("Not found $obj", contains(actual, obj))
    }
}

private fun <T> contains(actual: ArrayList<T>, obj: T) = actual.find { matches(it, obj) } != null

private fun <T> matches(lh: T, rh: T) = when (lh) {
    is Group -> matches(lh, rh as Group)
    is Author -> matches(lh, rh as Author)
    is Track -> matches(lh, rh as Track)
    else -> {
        fail("Unknown type of $lh")
        false
    }
}

private fun matches(lh: Group, rh: Group) =
    lh.id == rh.id && lh.name == rh.name

private fun matches(lh: Author, rh: Author) =
    lh.id == rh.id && lh.handle == rh.handle && lh.realName == rh.realName

private fun matches(lh: Track, rh: Track) =
    lh.id == rh.id && lh.filename == rh.filename && lh.size == rh.size
