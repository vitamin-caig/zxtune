package app.zxtune.fs.provider

import android.net.Uri
import org.junit.Assert.assertEquals
import org.junit.Assert.fail
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

private const val PATH = "scheme:/path?with/query"
private const val ENCODED_PATH = "scheme%3A%2Fpath%3Fwith%2Fquery"

private const val SEARCH_QUERY = "to search"

private const val AUTHORITY = "content://app.zxtune.vfs"

private const val MIME_ITEM = "vnd.android.cursor.item/vnd.app.zxtune.vfs.item"
private const val MIME_GROUP = "vnd.android.cursor.dir/vnd.app.zxtune.vfs.item"
private const val MIME_SIMPLEITEMS = "vnd.android.cursor.dir/vnd.app.zxtune.vfs.simple_item"

@RunWith(RobolectricTestRunner::class)
class QueryTest {

    @Test
    fun `test resolve uri`() {
        Uri.parse("${AUTHORITY}/resolve/${ENCODED_PATH}").let { uri ->
            assertEquals(MIME_ITEM, Query.mimeTypeOf(uri))
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertEquals(Query.TYPE_RESOLVE, Query.getUriType(uri))
            assertEquals(uri, Query.resolveUriFor(Uri.parse(PATH)))
        }
        Uri.parse("${AUTHORITY}/resolve").let { uri ->
            assertEquals(MIME_ITEM, Query.mimeTypeOf(uri))
            assertEquals(Uri.EMPTY, Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertEquals(Query.TYPE_RESOLVE, Query.getUriType(uri))
        }
    }

    @Test
    fun `test listing uri`() {
        Uri.parse("${AUTHORITY}/listing/${ENCODED_PATH}").let { uri ->
            assertEquals(MIME_GROUP, Query.mimeTypeOf(uri))
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertEquals(Query.TYPE_LISTING, Query.getUriType(uri))
            assertEquals(uri, Query.listingUriFor(Uri.parse(PATH)))
        }
        Uri.parse("${AUTHORITY}/listing").let { uri ->
            assertEquals(MIME_GROUP, Query.mimeTypeOf(uri))
            assertEquals(Uri.EMPTY, Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertEquals(Query.TYPE_LISTING, Query.getUriType(uri))
        }
    }

    @Test
    fun `test parents uri`() {
        Uri.parse("${AUTHORITY}/parents/${ENCODED_PATH}").let { uri ->
            assertEquals(MIME_SIMPLEITEMS, Query.mimeTypeOf(uri))
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertEquals(Query.TYPE_PARENTS, Query.getUriType(uri))
            assertEquals(uri, Query.parentsUriFor(Uri.parse(PATH)))
        }
        Uri.parse("${AUTHORITY}/parents").let { uri ->
            assertEquals(MIME_SIMPLEITEMS, Query.mimeTypeOf(uri))
            assertEquals(Uri.EMPTY, Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertEquals(Query.TYPE_PARENTS, Query.getUriType(uri))
        }
    }

    @Test
    fun `test search uri`() {
        Uri.parse("${AUTHORITY}/search/${ENCODED_PATH}?query=to%20search").let { uri ->
            assertEquals(MIME_GROUP, Query.mimeTypeOf(uri))
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertEquals(SEARCH_QUERY, Query.getQueryFrom(uri))
            assertEquals(Query.TYPE_SEARCH, Query.getUriType(uri))
            assertEquals(uri, Query.searchUriFor(Uri.parse(PATH), SEARCH_QUERY))
        }
        Uri.parse("${AUTHORITY}/search/${ENCODED_PATH}").let { uri ->
            assertEquals(MIME_GROUP, Query.mimeTypeOf(uri))
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertEquals(Query.TYPE_SEARCH, Query.getUriType(uri))
        }
        Uri.parse("${AUTHORITY}/search").let { uri ->
            assertThrowsIllegalArgumentException { Query.mimeTypeOf(uri) }
            assertThrowsIllegalArgumentException { Query.getPathFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertEquals(-1, Query.getUriType(uri))
        }
    }

    @Test
    fun `test file uri`() {
        Uri.parse("${AUTHORITY}/file/${ENCODED_PATH}").let { uri ->
            assertEquals(MIME_ITEM, Query.mimeTypeOf(uri))
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertEquals(Query.TYPE_FILE, Query.getUriType(uri))
            assertEquals(uri, Query.fileUriFor(Uri.parse(PATH)))
        }
        Uri.parse("${AUTHORITY}/file").let { uri ->
            assertThrowsIllegalArgumentException { Query.mimeTypeOf(uri) }
            assertThrowsIllegalArgumentException { Query.getPathFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertEquals(-1, Query.getUriType(uri))
        }
    }
}

private fun assertThrowsIllegalArgumentException(block: () -> Unit) = try {
    block()
    fail("Unexpected")
} catch (e: IllegalArgumentException) {
    e
}
