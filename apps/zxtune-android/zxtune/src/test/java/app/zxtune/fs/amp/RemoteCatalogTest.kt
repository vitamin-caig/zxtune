package app.zxtune.fs.amp

import app.zxtune.BuildConfig
import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.AdditionalMatchers.gt
import org.mockito.kotlin.any
import org.mockito.kotlin.argThat
import org.mockito.kotlin.atLeast
import org.mockito.kotlin.mock
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
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
        val visitor = mock<Catalog.GroupsVisitor>()
        catalog.queryGroups(visitor)
        val groupsApprox = 8800
        verify(visitor).accept(Group(6018, "0-Dezign"))
        verify(visitor).accept(Group(6013, "ABSENCE"))
        // multiline
        verify(visitor).accept(Group(677, "Anexia"))
        // utf-8
        verify(visitor).accept(Group(9016, "Åltalános Védelmi Hiba (AHV)"))
        // count
        verify(visitor, atLeast(groupsApprox)).accept(any())
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test authors by handle`() {
        val visitor = mock<Catalog.AuthorsVisitor>()
        catalog.queryAuthors("0-9", visitor)
        val authorsApprox = 50
        // Pagination with unknown size
        verify(visitor).accept(Author(9913, "00d", "Petteri Kuittinen"))
        // pivot
        verify(visitor).accept(Author(10, "4-Mat", "Matthew Simmonds"))
        // n/a in real name -> empty
        verify(visitor).accept(Author(15226, "808-Style", ""))
        // utf-8
        verify(visitor).accept(Author(13302, "8bitbubsy", "Olav Sørensen"))
        // count
        verify(visitor, atLeast(authorsApprox)).accept(any())
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test authors by country`() {
        val visitor = mock<Catalog.AuthorsVisitor>()
        catalog.queryAuthors(Country(44, "Russia"), visitor)
        val authorsApprox = 200
        verify(visitor).accept(Author(13606, "Agent Orange", "Oleg Sharonov"))
        verify(visitor).accept(Author(2142, "Doublestar", "Victor Shimyakov"))
        verify(visitor).accept(Author(8482, "Zyz", "Dmitry Rusak"))
        // count
        verify(visitor, atLeast(authorsApprox)).accept(any())
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test authors by group`() {
        val visitor = mock<Catalog.AuthorsVisitor>()
        catalog.queryAuthors(Group(1770, "Farbrausch"), visitor)
        val authorsApprox = 10
        verify(visitor).accept(Author(14354, "BeRo", "Benjamin Rosseaux"))
        verify(visitor).accept(Author(8120, "Wayfinder", "Sebastian Grillmaier"))
        // count
        verify(visitor, atLeast(authorsApprox)).accept(any())
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test authors tracks`() {
        val visitor = mock<Catalog.TracksVisitor>()
        catalog.queryTracks(Author(2085, "doh", "Nicolas Dessesart"), visitor)
        val tracksApprox = 160
        verify(visitor).accept(Track(15892, "egometriosporasie", 186))
        verify(visitor).accept(Track(15934, "un peu + a l'ouest", 309))
        verify(visitor).accept(Track(15804, "yapleindmondalagas", 190))
        // count
        verify(visitor, atLeast(tracksApprox)).accept(any())
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test search`() {
        val visitor = mock<Catalog.FoundTracksVisitor>()
        catalog.findTracks("zzz", visitor)
        val tracksApprox = 50
        verify(visitor).accept(Author(8462, "Zuo", ""), Track(121325, "\"bzzzt-chip 4\"", 7))
        verify(visitor).accept(Author(3993, "ADKD", ""), Track(165885, "frizzz", 12))
        verify(visitor).accept(Author(3993, "ADKD", ""), Track(172349, "frizzz.alt", 8))
        // last page
        verify(visitor).accept(Author(1669, "Daze", ""), Track(22313, "zzzzzzzzz", 50))
        verify(visitor, atLeast(tracksApprox)).accept(any(), any())
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test getTrackUris`() = with(RemoteCatalog.getTrackUris(12345)) {
        assertEquals(2L, size.toLong())
        assertEquals("${BuildConfig.CDN_ROOT}/download/amp/ids/12345", get(0).toString())
        assertEquals("https://amp.dascene.net/downmod.php?index=12345", get(1).toString())
    }

    @Test
    fun `test getPictureUris`() =
        with(RemoteCatalog.getPictureUris("/pictures/K/Karsten Koch - 3980/Karsten Koch.jpg")) {
            assertEquals(1L, size.toLong())
            assertEquals(
                "https://amp.dascene.net/pictures/K/Karsten%20Koch%20-%203980/Karsten%20Koch.jpg",
                get(0).toString()
            )
        }
}
