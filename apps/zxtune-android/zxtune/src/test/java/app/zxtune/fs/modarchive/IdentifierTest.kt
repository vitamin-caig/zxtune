package app.zxtune.fs.modarchive

import android.net.Uri
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class IdentifierTest {

    @Test
    fun `test root`() {
        testParse("modarchive:")
    }

    @Test
    fun `test authors`() {
        testParse("modarchive:/Author", category = Identifier.CATEGORY_AUTHOR)
        testParse(
            "modarchive:/Author/Of?author=12",
            category = Identifier.CATEGORY_AUTHOR,
            author = Author(12, "Of")
        )
        testParse(
            "modarchive:/Author/Unknown/Sorry?author=34&track=56",
            category = Identifier.CATEGORY_AUTHOR,
            author = Author(34, "Unknown"),
            track = Track(56, "Sorry", "", 0)
        )
    }

    @Test
    fun `test genres`() {
        testParse("modarchive:/Genre", category = Identifier.CATEGORY_GENRE)
        testParse(
            "modarchive:/Genre/Jazz?genre=78",
            category = Identifier.CATEGORY_GENRE,
            genre = Genre(78, "Jazz", 0)
        )
        testParse(
            "modarchive:/Genre/Rock/Roll?genre=90&track=12",
            category = Identifier.CATEGORY_GENRE,
            genre = Genre(90, "Rock", 0),
            track = Track(12, "Roll", "", 0)
        )
    }

    @Test
    fun `test random`() {
        testParse("modarchive:/Random", category = Identifier.CATEGORY_RANDOM)
        testParse(
            "modarchive:/Random/Track?track=34",
            category = Identifier.CATEGORY_RANDOM,
            track = Track(34, "Track", "", 0)
        )
    }

    @Test
    fun `test errors`() {
        // Scheme validity is checked at another level
        testAnalyze(
            "mdrch:/Category/Subcategory/Track?cat=34&track=56",
            isFromRoot = false,
            category = "Category",
            track = Track(56, "Track", "", 0)
        )
        // track filename is the last path component
        testAnalyze(
            "mdrch:/L/O/N/G/W/A/Y?cat=78&track=90",
            isFromRoot = false,
            category = "L",
            track = Track(90, "Y", "", 0)
        )
        testAnalyze(
            "mdrch:/C/S/T?genre=no&author=no&track=no",
            isFromRoot = false,
            category = "C"
        )
    }
}

private fun testParse(
    uri: String,
    category: String? = null,
    author: Author? = null,
    genre: Genre? = null,
    track: Track? = null
) {
    testAnalyze(
        uri = uri,
        isFromRoot = true,
        category = category,
        author = author,
        genre = genre,
        track = track
    )
    var builder = when {
        author != null -> Identifier.forAuthor(author)
        genre != null -> Identifier.forGenre(genre)
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
    genre: Genre? = null,
    track: Track? = null
) = Uri.parse(uri).let { url ->
    val path = url.pathSegments
    assertEquals("isFromRoot", isFromRoot, Identifier.isFromRoot(url))
    assertEquals("findCategory", category, Identifier.findCategory(path))
    assertEquals("findAuthor", author, Identifier.findAuthor(url, path))
    assertEquals("findGenre", genre, Identifier.findGenre(url, path))
    assertEquals("findTrack", track, Identifier.findTrack(url, path))
}
