package app.zxtune.fs.zxtunes

import android.net.Uri
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class IdentifierTest {

    @Test
    fun `test root`() {
        testParse("zxtunes:")
    }

    @Test
    fun `test authors`() {
        testParse("zxtunes:/Authors", category = Identifier.CATEGORY_AUTHORS)
        testParse(
            "zxtunes:/Authors/First?author=12", category = Identifier.CATEGORY_AUTHORS,
            author = Author(12, "First")
        )
        testParse(
            "zxtunes:/Authors/Second/1999?author=34", category = Identifier.CATEGORY_AUTHORS,
            author = Author(34, "Second"),
            date = 1999
        )
        testParse(
            "zxtunes:/Authors/Third/track.pt2?author=56&track=78",
            category = Identifier.CATEGORY_AUTHORS,
            author = Author(56, "Third"),
            track = Track(78, "track.pt2")
        )
        testParse(
            "zxtunes:/Authors/Fourth/2013/track.pt3?author=90&track=123",
            category = Identifier.CATEGORY_AUTHORS,
            author = Author(90, "Fourth"),
            date = 2013,
            track = Track(123, "track.pt3", "", null, 2013)
        )
    }

    @Test
    fun `test errors`() {
        // Scheme validity is checked at another level
        testAnalyze(
            "ztune:/Avtorz/Author/Track?author=345&track=56",
            isFromRoot = false,
            category = "Avtorz",
            author = Author(345, "Author"),
            track = Track(56, "Track")
        )
        testAnalyze(
            "ztune:/A/B/C/D?author=78&track=90",
            isFromRoot = false,
            category = "A",
            author = Author(78, "B")
        )
        testAnalyze("ztune:/A/B/C?author=number&track=number", isFromRoot = false, category = "A")
        testAnalyze(
            "ztune:/A/B/12345?author=12",
            isFromRoot = false,
            category = "A",
            author = Author(12, "B")
        );
    }
}

private fun testParse(
    uri: String,
    category: String? = null,
    author: Author? = null,
    date: Int? = null,
    track: Track? = null
) {
    testAnalyze(
        uri = uri,
        isFromRoot = true,
        category = category,
        author = author,
        date = date,
        track = track
    )
    var builder = when {
        author != null && date != null && track == null -> Identifier.forAuthor(author, date)
        author != null -> Identifier.forAuthor(author)
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
    date: Int? = null,
    track: Track? = null
) = Uri.parse(uri).let { url ->
    val path = url.pathSegments
    assertEquals("isFromRoot", isFromRoot, Identifier.isFromRoot(url))
    assertEquals("findCategory", category, Identifier.findCategory(path))
    assertEquals("findAuthor", author, Identifier.findAuthor(url, path))
    assertEquals("findDate", date, Identifier.findDate(url, path))
    assertEquals("findTrack", track, Identifier.findTrack(url, path))
}
