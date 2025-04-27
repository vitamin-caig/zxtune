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

private const val FILE_SIZE = 12345L

private const val AUTHORITY = "content://app.zxtune.vfs"

private const val MIME_ITEM = "vnd.android.cursor.item/vnd.app.zxtune.vfs.item"
private const val MIME_GROUP = "vnd.android.cursor.dir/vnd.app.zxtune.vfs.item"
private const val MIME_SIMPLEITEMS = "vnd.android.cursor.dir/vnd.app.zxtune.vfs.simple_item"
private const val MIME_NOTIFICATION = "vnd.android.cursor.item/vnd.app.zxtune.vfs.notification"

@RunWith(RobolectricTestRunner::class)
class QueryTest {

    @Test
    fun `test resolve uri`() {
        assertEquals(MIME_ITEM, Query.Type.RESOLVE.mime)
        Uri.parse("${AUTHORITY}/resolve/${ENCODED_PATH}").let { uri ->
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getSizeFrom(uri) }
            assertEquals(Query.Type.RESOLVE, Query.getUriType(uri))
            assertEquals(uri, Query.resolveUriFor(Uri.parse(PATH)))
        }
        Uri.parse("${AUTHORITY}/resolve").let { uri ->
            assertEquals(Uri.EMPTY, Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getSizeFrom(uri) }
            assertEquals(Query.Type.RESOLVE, Query.getUriType(uri))
        }
    }

    @Test
    fun `test listing uri`() {
        assertEquals(MIME_GROUP, Query.Type.LISTING.mime)
        Uri.parse("${AUTHORITY}/listing/${ENCODED_PATH}").let { uri ->
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getSizeFrom(uri) }
            assertEquals(Query.Type.LISTING, Query.getUriType(uri))
            assertEquals(uri, Query.listingUriFor(Uri.parse(PATH)))
        }
        Uri.parse("${AUTHORITY}/listing").let { uri ->
            assertEquals(Uri.EMPTY, Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getSizeFrom(uri) }
            assertEquals(Query.Type.LISTING, Query.getUriType(uri))
        }
    }

    @Test
    fun `test parents uri`() {
        assertEquals(MIME_SIMPLEITEMS, Query.Type.PARENTS.mime)
        Uri.parse("${AUTHORITY}/parents/${ENCODED_PATH}").let { uri ->
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getSizeFrom(uri) }
            assertEquals(Query.Type.PARENTS, Query.getUriType(uri))
            assertEquals(uri, Query.parentsUriFor(Uri.parse(PATH)))
        }
        Uri.parse("${AUTHORITY}/parents").let { uri ->
            assertEquals(Uri.EMPTY, Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getSizeFrom(uri) }
            assertEquals(Query.Type.PARENTS, Query.getUriType(uri))
        }
    }

    @Test
    fun `test search uri`() {
        assertEquals(MIME_GROUP, Query.Type.SEARCH.mime)
        Uri.parse("${AUTHORITY}/search/${ENCODED_PATH}?query=to%20search").let { uri ->
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertEquals(SEARCH_QUERY, Query.getQueryFrom(uri))
            assertThrowsIllegalArgumentException { Query.getSizeFrom(uri) }
            assertEquals(Query.Type.SEARCH, Query.getUriType(uri))
            assertEquals(uri, Query.searchUriFor(Uri.parse(PATH), SEARCH_QUERY))
        }
        Uri.parse("${AUTHORITY}/search/${ENCODED_PATH}").let { uri ->
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getSizeFrom(uri) }
            assertEquals(Query.Type.SEARCH, Query.getUriType(uri))
        }
        Uri.parse("${AUTHORITY}/search").let { uri ->
            assertEquals(Uri.EMPTY, Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getSizeFrom(uri) }
            assertEquals(Query.Type.SEARCH, Query.getUriType(uri))
        }
    }

    @Test
    fun `test file uri`() {
        assertEquals(MIME_ITEM, Query.Type.FILE.mime)
        Uri.parse("${AUTHORITY}/file").let { uri ->
            assertEquals(Uri.EMPTY, Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getSizeFrom(uri) }
            assertEquals(Query.Type.FILE, Query.getUriType(uri))
        }
        Uri.parse("${AUTHORITY}/file/${ENCODED_PATH}").let { uri ->
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getSizeFrom(uri) }
            assertEquals(Query.Type.FILE, Query.getUriType(uri))
        }
        Uri.parse("${AUTHORITY}/file/${ENCODED_PATH}?size=${FILE_SIZE}").let { uri ->
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertEquals(FILE_SIZE, Query.getSizeFrom(uri))
            assertEquals(Query.Type.FILE, Query.getUriType(uri))
            assertEquals(uri, Query.fileUriFor(Uri.parse(PATH), FILE_SIZE))
        }
    }

    @Test
    fun `test notification uri`() {
        assertEquals(MIME_NOTIFICATION, Query.Type.NOTIFICATION.mime)
        Uri.parse("${AUTHORITY}/notification/${ENCODED_PATH}").let { uri ->
            assertEquals(Uri.parse(PATH), Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getSizeFrom(uri) }
            assertEquals(Query.Type.NOTIFICATION, Query.getUriType(uri))
            assertEquals(uri, Query.notificationUriFor(Uri.parse(PATH)))
        }
        Uri.parse("${AUTHORITY}/notification").let { uri ->
            assertEquals(Uri.EMPTY, Query.getPathFrom(uri))
            assertThrowsIllegalArgumentException { Query.getQueryFrom(uri) }
            assertThrowsIllegalArgumentException { Query.getSizeFrom(uri) }
            assertEquals(Query.Type.NOTIFICATION, Query.getUriType(uri))
        }
    }
}

private fun assertThrowsIllegalArgumentException(block: () -> Unit) = try {
    block()
    fail("Unexpected")
} catch (e: IllegalArgumentException) {
    e
}
