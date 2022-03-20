package app.zxtune.fs.zxart

import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner

private fun matchesTrack(
    id: Int,
    filename: String,
    title: String,
    duration: String,
    year: Int = 0,
    compo: String = "",
    partyplace: Int = 0
): (Track) -> Boolean = { argument ->
    argument.id == id
            && argument.filename == filename
            && argument.title == title
            && argument.duration == duration
            && argument.year == year
            && argument.compo == compo
            && argument.partyplace == partyplace
            && argument.votes.toDouble() <= 5.0
}

@RunWith(RobolectricTestRunner::class)
class RemoteCatalogTest {

    private lateinit var catalog: RemoteCatalog

    @Before
    fun setUp() = with(HttpProviderFactory.createTestProvider()) {
        catalog = RemoteCatalog(MultisourceHttpProvider(this))
    }

    @Test
    fun `test queryAuthors`() {
        val visitor = mock<Catalog.AuthorsVisitor>()

        // https://zxart.ee/zxtune/language:eng/action:authors
        catalog.queryAuthors(visitor)

        verify(visitor).setCountHint(argThat { hint -> hint > 1270 })
        verify(visitor).accept(Author(57761, "!de", ""))
        verify(visitor).accept(Author(20333, "4mat", "Matthew Simmonds"))
        verify(visitor).accept(Author(85837, "Sim&Sim", "Igor Dmitriev, Sergey Dmitriev"))
        verify(visitor).accept(Author(40232, "Ян Лисицкий", "Yan Lisitskiy"))
    }

    @Test
    fun `test queryAuthorTracks`() {
        val visitor = mock<Catalog.TracksVisitor>()

        // https://zxart.ee/zxtune/language:eng/action:tunes/authorId:40406
        catalog.queryAuthorTracks(Author(40406, "UNUSED", "UNUSED"), visitor)

        verify(visitor).setCountHint(argThat { hint -> hint > 110 })
        // internal title
        verify(visitor).accept(
            argThat(
                matchesTrack(
                    78258,
                    "Alex Rostov - ARISE (1998) (FunTop 1998, 18).pt2",
                    "ARISE BY ALEX ROSTOV/DEMENTIA",
                    "3:03.24",
                    1998,
                    "ay",
                    18
                )
            )
        )
        // full internal title
        verify(visitor).accept(
            argThat(
                matchesTrack(
                    90515,
                    "Alex Rostov - Aurora (2009).pt3",
                    "Alexander Rostov&#039;DT 2009 - Aurora (Enlight style) AY/ABC",
                    "1:38.14",
                    2009,
                    "standard"
                )
            )
        )
        // generic title, no year, no compo
        verify(visitor).accept(
            argThat(
                matchesTrack(
                    78271,
                    "Alex Rostov - Dementia.stc",
                    "Dementia",
                    "0:07.42",
                    0
                )
            )
        )
    }

    @Test
    fun `test queryParties`() {
        val visitor = mock<Catalog.PartiesVisitor>()

        // https://zxart.ee/zxtune/language:eng/action:parties
        catalog.queryParties(visitor)

        verify(visitor).setCountHint(argThat { hint -> hint > 180 })
        verify(visitor).accept(Party(12351, "3BM OpenAir 2012", 2012))
        verify(visitor).accept(
            Party(
                50251,
                "BotB OHC \"Locked In The Classroom With No Ammo Left\"",
                2014
            )
        )
    }

    @Test
    fun `test queryPartyTracks`() {
        val visitor = mock<Catalog.TracksVisitor>()

        // https://zxart.ee/zxtune/language:eng/action:tunes/partyId:4045
        catalog.queryPartyTracks(Party(4045, "UNUSED", -100), visitor)

        verify(visitor).setCountHint(argThat { hint -> hint > 20 })
        verify(visitor).accept(
            argThat(
                matchesTrack(
                    44800,
                    "Mast - Activity (1999) (Paradox 1999, 14).pt3",
                    "Mast/FtL 8.07.99 - Activity",
                    "2:31.07",
                    1999,
                    "ay",
                    14
                )
            )
        )
        verify(visitor).accept(
            argThat(
                matchesTrack(
                    44324,
                    "Shov - Miles New (1999) (Paradox 1999, 3).pt3",
                    "Miles New",
                    "3:43.31",
                    1999,
                    "ay",
                    3
                )
            )
        )
    }

    @Test
    fun `test queryTopTracks`() {
        val visitor = mock<Catalog.TracksVisitor>()

        // https://zxart.ee/zxtune/language:eng/action:topTunes/limit:10
        val limit = 10
        catalog.queryTopTracks(limit, visitor)

        // TODO: fix to return limit
        verify(visitor).setCountHint(argThat { hint -> hint > 25000 })
        verify(visitor, times(limit)).accept(any())
        verify(visitor).accept(
            argThat(
                matchesTrack(
                    74340,
                    "MmcM - AsSuRed (2000).pt3",
                    "Mm&lt;M of Sage 14.Apr.XX twr 00:37 - AsSuRed ... Hi! My Frends ...",
                    "3:46.31",
                    2000
                )
            )
        )
        verify(visitor).accept(
            argThat(
                matchesTrack(
                    192733,
                    "Karbofos - шторм (2017) (ArtField 2017, 1).pt3",
                    "karbo 2o17 ymabc - ?????",
                    "2:32.23",
                    2017,
                    "standard",
                    1
                )
            )
        )
    }

    @Test
    fun `test findTracks`() {
        val visitor = mock<Catalog.FoundTracksVisitor>()

        // https://zxart.ee/zxtune/language:eng/action:search/query:test
        catalog.findTracks("test", visitor)

        verify(visitor).setCountHint(argThat { hint -> hint > 50 })
        verify(visitor).accept(
            eq(Author(38066, "Author38066", "")), argThat(
                matchesTrack(
                    75480,
                    "Moonwalker - 10MinutesTrash.pt2",
                    "&quot;10MinutesTrash&quot; by Moonwalker",
                    "0:49.07"
                )
            )
        )
    }

    @Test
    fun `test getTrackUris()`() = with(RemoteCatalog.getTrackUris(12345)) {
        assertEquals(1L, size.toLong())
        assertEquals("https://zxart.ee/file/id:12345", get(0).toString())
    }
}
