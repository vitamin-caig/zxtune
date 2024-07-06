package app.zxtune.fs.zxart

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
import org.mockito.kotlin.eq
import org.mockito.kotlin.mock
import org.mockito.kotlin.times
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
    fun `test queryAuthors`() {
        val visitor = mock<Catalog.AuthorsVisitor>()
        val authorsMin = 1270
        // https://zxart.ee/zxtune/language:eng/action:authors
        catalog.queryAuthors(visitor)

        verify(visitor).setCountHint(gt(authorsMin))
        verify(visitor).accept(Author(57761, "!de", ""))
        verify(visitor).accept(Author(20333, "4-Mat", "Matthew Simmonds"))
        verify(visitor).accept(Author(85837, "Sim&Sim", "Igor Dmitriev, Sergey Dmitriev"))
        verify(visitor).accept(Author(40232, "Ян Лисицкий", "Yan Lisitskiy"))
        verify(visitor, atLeast(authorsMin)).accept(any())
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test queryAuthorTracks`() {
        val visitor = mock<Catalog.TracksVisitor>()
        val tracksMin = 110
        // https://zxart.ee/zxtune/language:eng/action:tunes/authorId:40406
        catalog.queryAuthorTracks(Author(40406, "UNUSED", "UNUSED"), visitor)

        verify(visitor).setCountHint(gt(tracksMin))
        // internal title
        verify(visitor).accept(argThat {
            matches(
                78258,
                "Alex Rostov - ARISE (1998) (FunTop 1998, 18).pt2",
                "ARISE BY ALEX ROSTOV/DEMENTIA",
                "3:03.24",
                1998,
                "ay",
                18
            )
        })
        // full internal title
        verify(visitor).accept(argThat {
            matches(
                90515,
                "Alex Rostov - Aurora (2009).pt3",
                "Alexander Rostov&#039;DT 2009 - Aurora (Enlight style) AY/ABC",
                "1:38.14",
                2009,
                "standard"
            )
        })
        // generic title, no year, no compo
        verify(visitor).accept(argThat {
            matches(
                78271, "Alex Rostov - Dementia.stc", "Dementia", "0:07.42", 0
            )
        })
        verify(visitor, atLeast(tracksMin)).accept(any())
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test queryParties`() {
        val visitor = mock<Catalog.PartiesVisitor>()
        val partiesMin = 180

        // https://zxart.ee/zxtune/language:eng/action:parties
        catalog.queryParties(visitor)

        verify(visitor).setCountHint(gt(partiesMin))
        verify(visitor).accept(Party(12351, "3BM OpenAir 2012", 2012))
        verify(visitor).accept(
            Party(50251, "BotB OHC \"Locked In The Classroom With No Ammo Left\"", 2014)
        )
        verify(visitor, atLeast(partiesMin)).accept(any())
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test queryPartyTracks`() {
        val visitor = mock<Catalog.TracksVisitor>()
        val tracksMin = 20

        // https://zxart.ee/zxtune/language:eng/action:tunes/partyId:4045
        catalog.queryPartyTracks(Party(4045, "UNUSED", -100), visitor)

        verify(visitor).setCountHint(gt(tracksMin))
        verify(visitor).accept(argThat {
            matches(
                44800,
                "Mast - Activity (1999) (Paradox 1999, 14).pt3",
                "Mast/FtL 8.07.99 - Activity",
                "2:31.07",
                1999,
                "ay",
                14
            )
        })
        verify(visitor).accept(argThat {
            matches(
                44324,
                "Shov - Miles New (1999) (Paradox 1999, 3).pt3",
                "Miles New",
                "3:43.31",
                1999,
                "ay",
                3
            )
        })
        verify(visitor, atLeast(tracksMin)).accept(any())
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test queryTopTracks`() {
        val visitor = mock<Catalog.TracksVisitor>()

        // https://zxart.ee/zxtune/language:eng/action:topTunes/limit:10
        val limit = 20
        catalog.queryTopTracks(limit, visitor)

        // TODO: fix to return limit
        verify(visitor).setCountHint(gt(25000))
        verify(visitor, times(limit)).accept(any())
        verify(visitor).accept(argThat {
            matches(
                74340,
                "MmcM - AsSuRed (2000).pt3",
                "Mm&lt;M of Sage 14.Apr.XX twr 00:37 - AsSuRed ... Hi! My Frends ...",
                "3:46.31",
                2000
            )
        })
        verify(visitor).accept(argThat {
            matches(
                192733,
                "Karbofos - шторм (2017) (ArtField 2017, 1).pt3",
                "karbo 2o17 ymabc - ?????",
                "2:32.23",
                2017,
                "standard",
                1
            )
        })
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test findTracks`() {
        val visitor = mock<Catalog.FoundTracksVisitor>()
        val tracksMin = 50

        // https://zxart.ee/zxtune/language:eng/action:search/query:test
        catalog.findTracks("test", visitor)

        verify(visitor).setCountHint(gt(tracksMin))
        verify(visitor).accept(eq(Author(391481, "Author391481", "")), argThat {
            matches(
                75480,
                "Moonwalker - 10MinutesTrash.pt2",
                "&quot;10MinutesTrash&quot; by Moonwalker",
                "0:49.07"
            )
        })
        verify(visitor, atLeast(tracksMin)).accept(any(), any())
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test getTrackUris()`() = with(RemoteCatalog.getTrackUris(12345)) {
        assertEquals(1L, size.toLong())
        assertEquals("https://zxart.ee/file/id:12345", get(0).toString())
    }
}

private fun Track.matches(
    id: Int,
    filename: String,
    title: String,
    duration: String,
    year: Int = 0,
    compo: String = "",
    partyplace: Int = 0
) =
    this.id == id && this.filename == filename && this.title == title && this.duration == duration && this.year == year && this.compo == compo && this.partyplace == partyplace && this.votes.toDouble() <= 5.0

