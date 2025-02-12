package app.zxtune.fs.amp

import android.net.Uri
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class IdentifierTest {

    @Test
    fun `test root`() {
        testParse("amp:")
    }

    @Test
    fun `test handles`() {
        testParse("amp:/Handle", category = Identifier.CATEGORY_HANDLE)
        testParse("amp:/Handle/A", category = Identifier.CATEGORY_HANDLE, handleLetter = "A")
        testParse("amp:/Handle/0-9", category = Identifier.CATEGORY_HANDLE, handleLetter = "0-9")
        testParse(
            "amp:/Handle/F/Fd?author=12",
            category = Identifier.CATEGORY_HANDLE,
            handleLetter = "F",
            author = Author(12, "Fd", "")
        )
        testParse(
            "amp:/Handle/M/Me/nothing?author=23&track=34",
            category = Identifier.CATEGORY_HANDLE,
            handleLetter = "M",
            author = Author(23, "Me", ""),
            track = Track(34, "nothing", 0)
        )
    }

    @Test
    fun `test countries`() {
        testParse("amp:/Country", category = Identifier.CATEGORY_COUNTRY)
        testParse(
            "amp:/Country/Oz?country=56",
            category = Identifier.CATEGORY_COUNTRY,
            country = Country(56, "Oz")
        )
        testParse(
            "amp:/Country/Atlantis/Giant?country=67&author=78",
            category = Identifier.CATEGORY_COUNTRY,
            country = Country(67, "Atlantis"),
            author = Author(78, "Giant", "")
        )
        testParse(
            "amp:/Country/Limonia/Duna/Flat.pt2?country=89&author=1&track=23",
            category = Identifier.CATEGORY_COUNTRY,
            country = Country(89, "Limonia"),
            author = Author(1, "Duna", ""),
            track = Track(23, "Flat.pt2", 0)
        )
    }

    @Test
    fun `test groups`() {
        testParse("amp:/Group", category = Identifier.CATEGORY_GROUP)
        testParse(
            "amp:/Group/Support?group=45",
            category = Identifier.CATEGORY_GROUP,
            group = Group(45, "Support")
        )
        testParse(
            "amp:/Group/Of/Blood?group=67&author=89",
            category = Identifier.CATEGORY_GROUP,
            group = Group(67, "Of"),
            author = Author(89, "Blood", "")
        )
        testParse(
            "amp:/Group/Pen/Fuh/Rer.suxx?group=90&author=12&track=34",
            category = Identifier.CATEGORY_GROUP,
            group = Group(90, "Pen"),
            author = Author(12, "Fuh", ""),
            track = Track(34, "Rer.suxx", 0)
        )
    }

    @Test
    fun `test errors`() {
        // Scheme validity is checked at another level
        testAnalyze(
            "zamp:/Category/Subcategory/Author/Track?cat=12&author=34&track=56",
            isFromRoot = false,
            category = "Category",
            author = Author(34, "Author", ""),
            track = Track(56, "Track", 0)
        )
        testAnalyze(
            "vamp:/C/S/A/T?cat=no&author=no&track=trk",
            isFromRoot = false,
            category = "C",
            handleLetter = "S"
        )
    }
}

private fun testParse(
    uri: String,
    category: String? = null,
    handleLetter: String? = null,
    country: Country? = null,
    group: Group? = null,
    author: Author? = null,
    track: Track? = null,
) {
    testAnalyze(
        uri = uri,
        isFromRoot = true,
        category = category,
        handleLetter = handleLetter,
        country = country,
        group = group,
        author = author,
        track = track,
    )
    var builder = when {
        handleLetter != null -> Identifier.forHandleLetter(handleLetter)
        country != null -> Identifier.forCountry(country)
        group != null -> Identifier.forGroup(group)
        category != null -> Identifier.forCategory(category)
        else -> Identifier.forRoot()
    }
    if (author != null) {
        builder = Identifier.forAuthor(builder, author)
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
    handleLetter: String? = null,
    country: Country? = null,
    group: Group? = null,
    author: Author? = null,
    track: Track? = null,
) = Uri.parse(uri).let { url ->
    val path = url.pathSegments
    assertEquals("isFromRoot", isFromRoot, Identifier.isFromRoot(url))
    assertEquals("findCategory", category, Identifier.findCategory(path))
    assertEquals("findHandleLetter", handleLetter, Identifier.findHandleLetter(url, path))
    assertEquals("findCountry", country, Identifier.findCountry(url, path))
    assertEquals("findGroup", group, Identifier.findGroup(url, path))
    assertEquals("findAuthor", author, Identifier.findAuthor(url, path))
    assertEquals("findTrack", track, Identifier.findTrack(url, path))
}
