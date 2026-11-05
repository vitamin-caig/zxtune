package app.zxtune.fs.sndh

import android.net.Uri
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class IdentifierTest {

    @Test
    fun `test root`() {
        testParse("sndh:")
    }

    @Test
    fun `test authors`() {
        testParse("sndh:/Authors", category = Identifier.CATEGORY_AUTHORS)
        testParse(
            "sndh:/Authors/505",
            category = Identifier.CATEGORY_AUTHORS,
            author = Author("505")
        )
        testParse(
            "sndh:/Authors/505/052?track=2780",
            category = Identifier.CATEGORY_AUTHORS,
            author = Author("505"),
            track = Track(2780, "052", "")
        )
    }

    @Test
    fun `test random`() {
        testParse("sndh:/Random", category = Identifier.CATEGORY_RANDOM)
        testParse(
            "sndh:/Random/Hiya%20Placez?track=1526",
            category = Identifier.CATEGORY_RANDOM,
            track = Track(1526, "Hiya Placez", "")
        )
    }

    @Test
    fun `test special symbols`() {
        testRoundTrip(author = Author("#40E0D0"))
        testRoundTrip(author = Author("Bodenständig 2000"))
        testRoundTrip(author = Author("A&B"))
        testRoundTrip(author = Author("DF0:BAD"))
        testRoundTrip(
            author = Author("505"),
            track = Track(12345, "Some/Title?x#y", "")
        )
        testRoundTrip(track = Track(23456, "Track with spaces", ""))
    }

    @Test
    fun `test errors`() {
        testParse("other:/Authors", category = Identifier.CATEGORY_AUTHORS, isFromRoot = false)
        testParse("sndh:/Other/505?track=2780", category = "Other")
        testParse("sndh:/Authors/505/052", category = Identifier.CATEGORY_AUTHORS)
        testParse("sndh:/Random?track=1526", category = Identifier.CATEGORY_RANDOM)
        testParse(
            "sndh:/Random/Track?track=not-a-number",
            category = Identifier.CATEGORY_RANDOM
        )
    }
}

private fun testRoundTrip(author: Author? = null, track: Track? = null) {
    var builder = when {
        author != null -> Identifier.forAuthor(author)
        track != null -> Identifier.forCategory(Identifier.CATEGORY_RANDOM)
        else -> Identifier.forRoot()
    }
    if (track != null) {
        builder = Identifier.forTrack(builder, track)
    }
    val uri = builder.build()
    val path = uri.pathSegments
    assertEquals(author, Identifier.findAuthor(uri, path))
    assertEquals(track, Identifier.findTrack(uri, path))
}

private fun testParse(
    uri: String,
    category: String? = null,
    author: Author? = null,
    track: Track? = null,
    isFromRoot: Boolean = true,
) = Uri.parse(uri).let { url ->
    val path = url.pathSegments
    assertEquals(isFromRoot, Identifier.isFromRoot(url))
    assertEquals(category, Identifier.findCategory(path))
    assertEquals(author, Identifier.findAuthor(url, path))
    assertEquals(track, Identifier.findTrack(url, path))
}
