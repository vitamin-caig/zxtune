package app.zxtune.fs.zxart

import android.net.Uri
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class IdentifierTest {

    @Test
    fun `test root`() {
        testParse("zxart:")
    }

    @Test
    fun `test authors`() {
        testParse("zxart:/Authors", category = Identifier.CATEGORY_AUTHORS)
        testParse(
            "zxart:/Authors/Au?author=12",
            category = Identifier.CATEGORY_AUTHORS,
            author = Author(12, "Au", "")
        )
        testParse(
            "zxart:/Authors/Aut/2013?author=34",
            category = Identifier.CATEGORY_AUTHORS,
            author = Author(34, "Aut", ""),
            year = 2013
        )
        testParse(
            "zxart:/Authors/Autt/unknown?author=34",
            category = Identifier.CATEGORY_AUTHORS,
            author = Author(34, "Autt", ""),
            year = 0
        )
        testParse(
            "zxart:/Authors/Auto/2021/virus.pt3?author=56&track=789",
            category = Identifier.CATEGORY_AUTHORS,
            author = Author(56, "Auto", ""),
            year = 2021,
            track = Track(789, "virus.pt3"),
        )
    }

    @Test
    fun `test parties`() {
        testParse("zxart:/Parties", category = Identifier.CATEGORY_PARTIES)
        testParse("zxart:/Parties/1999", category = Identifier.CATEGORY_PARTIES, year = 1999)
        testParse(
            "zxart:/Parties/2018/CC?party=123", category = Identifier.CATEGORY_PARTIES,
            year = 2018, party = Party(123, "CC", 2018)
        )
        testParse(
            "zxart:/Parties/2020/DiHalt/AY?party=234", category = Identifier.CATEGORY_PARTIES,
            year = 2020, party = Party(234, "DiHalt", 2020), compo = "AY"
        )
        testParse(
            "zxart:/Parties/2019/Paradox/Wild/nothing.mp3?party=345&track=4567",
            category = Identifier.CATEGORY_PARTIES,
            year = 2019,
            party = Party(345, "Paradox", 2019),
            compo = "Wild",
            track = Track(4567, "nothing.mp3"),
        )
    }

    @Test
    fun `test top`() {
        testParse("zxart:/Top", category = Identifier.CATEGORY_TOP)
        testParse(
            "zxart:/Top/track.pt3?track=890",
            category = Identifier.CATEGORY_TOP,
            track = Track(890, "track.pt3")
        )
    }

    @Test
    fun `test errors`() {
        // Scheme validity is checked at another level
        testAnalyze(
            "zart:/Category/",
            isFromRoot = false,
            category = "Category"
        )
        // track is parsed
        testAnalyze(
            "zxart:/Authors/Invalid/year/track.pt3?author=number&track=123",
            category = Identifier.CATEGORY_AUTHORS,
            track = Track(123, "track.pt3")
        )
        testAnalyze(
            "zxart:/Parties/year/Party/Compo/track.pt3?party=number&track=456",
            category = Identifier.CATEGORY_PARTIES,
            compo = "Compo",
            track = Track(456, "track.pt3")
        )
        testAnalyze(
            "zxart:/Authors/Au/1000/track.pt3?author=789&track=number",
            category = Identifier.CATEGORY_AUTHORS,
            author = Author(789, "Au", ""),
            year = 1000
        )
        testAnalyze(
            "zxart:/Parties/2000/Party/Compa/track.pt3?party=123&track=number",
            category = Identifier.CATEGORY_PARTIES,
            year = 2000,
            party = Party(123, "Party", 2000),
            compo = "Compa"
        )
        testAnalyze(
            "zxart:/Top/track.pt3?track=number",
            category = Identifier.CATEGORY_TOP
        )
    }
}

private fun testParse(
    uri: String,
    category: String? = null,
    author: Author? = null,
    year: Int? = null,
    party: Party? = null,
    compo: String? = null,
    track: Track? = null,
) {
    testAnalyze(
        uri = uri,
        isFromRoot = true,
        category = category,
        author = author,
        year = year,
        party = party,
        compo = compo,
        track = track
    )
    var builder = when {
        author != null && year != null -> Identifier.forAuthor(author, year)
        author != null -> Identifier.forAuthor(author)
        year != null && party == null -> Identifier.forPartiesYear(year)
        party != null && compo != null -> Identifier.forPartyCompo(party, compo)
        party != null -> Identifier.forParty(party)
        category != null -> Identifier.forCategory(category)
        else -> Identifier.forRoot()
    }
    if (track != null) {
        builder = Identifier.forTrack(builder, track)
    }
    assertEquals("uri", uri, builder.toString())
}

private fun testAnalyze(
    uri: String,
    isFromRoot: Boolean = true,
    category: String? = null,
    author: Author? = null,
    year: Int? = null,
    party: Party? = null,
    compo: String? = null,
    track: Track? = null,
) = Uri.parse(uri).let { url ->
    val path = url.pathSegments
    assertEquals("isFromRoot", isFromRoot, Identifier.isFromRoot(url))
    assertEquals("findCategory", category, Identifier.findCategory(path))
    when (category) {
        Identifier.CATEGORY_AUTHORS -> {
            assertEquals("findAuthor", author, Identifier.findAuthor(url, path))
            assertEquals("findAuthorYear", year, Identifier.findAuthorYear(url, path))
            assertEquals("findAuthorTrack", track, Identifier.findAuthorTrack(url, path))
        }
        Identifier.CATEGORY_PARTIES -> {
            assertEquals("findParty", party, Identifier.findParty(url, path))
            assertEquals("findPartiesYear", year, Identifier.findPartiesYear(url, path))
            assertEquals("findPartyCompo", compo, Identifier.findPartyCompo(url, path))
            assertEquals("findPartyTrack", track, Identifier.findPartyTrack(url, path))
        }
        Identifier.CATEGORY_TOP -> {
            assertEquals("findTopTrack", track, Identifier.findTopTrack(url, path))
        }
    }
}
