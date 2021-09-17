package app.zxtune.fs.zxart

import androidx.test.ext.junit.runners.AndroidJUnit4
import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.ArgumentMatcher
import org.mockito.kotlin.*

private class MatchesTrack(
    private val id: Int,
    private val filename: String,
    private val title: String,
    private val duration: String,
    private val year: Int = 0,
    private val compo: String? = null,
    private val partyplace: Int = 0
) : ArgumentMatcher<Track> {
    override fun matches(argument: Track) = argument.id == id
            && argument.filename == filename
            && argument.title == title
            && argument.duration == duration
            && argument.year == year
            && argument.compo == compo
            && argument.partyplace == partyplace
            && argument.votes.toDouble() <= 5.0
}

@RunWith(AndroidJUnit4::class)
class RemoteCatalogTest {

    private lateinit var catalog: RemoteCatalog

    @Before
    fun setUp() = with(HttpProviderFactory.createTestProvider()) {
        catalog = RemoteCatalog(MultisourceHttpProvider(this))
    }

    @Test
    fun test_queryAuthors() {
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
    fun test_queryAuthorTracks() {
        val visitor = mock<Catalog.TracksVisitor>()

        // https://zxart.ee/zxtune/language:eng/action:tunes/authorId:40406
        catalog.queryAuthorTracks(Author(40406, "UNUSED", "UNUSED"), visitor)

        verify(visitor).setCountHint(argThat { hint -> hint > 110 })
        // internal title
        verify(visitor).accept(
            argThat(
                MatchesTrack(
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
                MatchesTrack(
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
                MatchesTrack(
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
    fun test_queryParties() {
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
    fun test_queryPartyTracks() {
        val visitor = mock<Catalog.TracksVisitor>()

        // https://zxart.ee/zxtune/language:eng/action:tunes/partyId:4045
        catalog.queryPartyTracks(Party(4045, "UNUSED", -100), visitor)

        verify(visitor).setCountHint(argThat { hint -> hint > 20 })
        verify(visitor).accept(
            argThat(
                MatchesTrack(
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
        verify(visitor).accept(argThat(
            MatchesTrack(
                44324,
                "Shov - Miles New (1999) (Paradox 1999, 3).pt3",
                "Miles New",
                "3:43.31",
                1999,
                "ay",
                3
            )
        ))
    }

    @Test
    fun test_queryTopTracks() {
        val visitor = mock<Catalog.TracksVisitor>()

        // https://zxart.ee/zxtune/language:eng/action:topTunes/limit:10
        val limit = 10
        catalog.queryTopTracks(limit, visitor)

        // TODO: fix to return limit
        verify(visitor).setCountHint(argThat { hint -> hint > 25000 })
        verify(visitor, times(limit)).accept(any())
        verify(visitor).accept(
            argThat(
                MatchesTrack(
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
                MatchesTrack(
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
    fun test_findTracks() {
        val visitor = mock<Catalog.FoundTracksVisitor>()

        // https://zxart.ee/zxtune/language:eng/action:search/query:test
        catalog.findTracks("test", visitor)

        verify(visitor).setCountHint(argThat { hint -> hint > 50 })
        verify(visitor).accept(
            eq(Author(38066, "Author38066", "")), argThat(
                MatchesTrack(
                    75480,
                    "Moonwalker - 10MinutesTrash.pt2",
                    "&quot;10MinutesTrash&quot; by Moonwalker",
                    "0:49.07"
                )
            )
        )
    }
}
