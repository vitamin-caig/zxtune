package app.zxtune.fs.provider

import android.net.Uri
import androidx.core.net.toUri
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
private const val MIME_NOTIFICATION = "vnd.android.cursor.item/vnd.app.zxtune.vfs.notification"

@RunWith(RobolectricTestRunner::class)
class QueryTest {

    @Test
    fun `test resolve uri`() {
        assertEquals(MIME_GROUP, Query.Type.RESOLVE.mime)
        with(Query.parse("${AUTHORITY}/resolve/${ENCODED_PATH}")) {
            assertEquals(Uri.parse(PATH), path)
            assertThrowsIllegalArgumentException { searchQuery }
            assertThrowsIllegalArgumentException { fileSize }
            assertEquals(Query.Type.RESOLVE, type)
            assertEquals(this, Query.forResolve(PATH.toUri()))
        }
        with(Query.parse("${AUTHORITY}/resolve")) {
            assertEquals(Uri.EMPTY, path)
            assertThrowsIllegalArgumentException { searchQuery }
            assertThrowsIllegalArgumentException { fileSize }
            assertEquals(Query.Type.RESOLVE, type)
        }
    }

    @Test
    fun `test listing uri`() {
        assertEquals(MIME_GROUP, Query.Type.LISTING.mime)
        with(Query.parse("${AUTHORITY}/listing/${ENCODED_PATH}")) {
            assertEquals(Uri.parse(PATH), path)
            assertThrowsIllegalArgumentException { searchQuery }
            assertThrowsIllegalArgumentException { fileSize }
            assertEquals(Query.Type.LISTING, type)
            assertEquals(this, Query.forListing(Uri.parse(PATH)))
        }
        with(Query.parse("${AUTHORITY}/listing")) {
            assertEquals(Uri.EMPTY, path)
            assertThrowsIllegalArgumentException { searchQuery }
            assertThrowsIllegalArgumentException { fileSize }
            assertEquals(Query.Type.LISTING, type)
        }
    }

    @Test
    fun `test feed uri`() {
        assertEquals(MIME_GROUP, Query.Type.FEED.mime)
        with(Query.parse("${AUTHORITY}/feed/${ENCODED_PATH}")) {
            assertEquals(Uri.parse(PATH), path)
            assertThrowsIllegalArgumentException { searchQuery }
            assertThrowsIllegalArgumentException { fileSize }
            assertEquals(Query.Type.FEED, type)
            assertEquals(this, Query.forFeed(Uri.parse(PATH)))
        }
        with(Query.parse("${AUTHORITY}/feed")) {
            assertEquals(Uri.EMPTY, path)
            assertThrowsIllegalArgumentException { searchQuery }
            assertThrowsIllegalArgumentException { fileSize }
            assertEquals(Query.Type.FEED, type)
        }
    }

    @Test
    fun `test search uri`() {
        assertEquals(MIME_GROUP, Query.Type.SEARCH.mime)
        with(Query.parse("${AUTHORITY}/search/${ENCODED_PATH}?query=to%20search")) {
            assertEquals(Uri.parse(PATH), path)
            assertEquals(SEARCH_QUERY, searchQuery)
            assertThrowsIllegalArgumentException { fileSize }
            assertEquals(Query.Type.SEARCH, type)
            assertEquals(this, Query.forSearch(Uri.parse(PATH), SEARCH_QUERY))
        }
        with(Query.parse("${AUTHORITY}/search/${ENCODED_PATH}")) {
            assertEquals(Uri.parse(PATH), path)
            assertThrowsIllegalArgumentException { searchQuery }
            assertThrowsIllegalArgumentException { fileSize }
            assertEquals(Query.Type.SEARCH, type)
        }
        with(Query.parse("${AUTHORITY}/search")) {
            assertEquals(Uri.EMPTY, path)
            assertThrowsIllegalArgumentException { searchQuery }
            assertThrowsIllegalArgumentException { fileSize }
            assertEquals(Query.Type.SEARCH, type)
        }
    }

    @Test
    fun `test file uri`() {
        assertEquals(MIME_ITEM, Query.Type.FILE.mime)
        with(Query.parse("${AUTHORITY}/file")) {
            assertEquals(Uri.EMPTY, path)
            assertThrowsIllegalArgumentException { searchQuery }
            assertThrowsIllegalArgumentException { fileSize }
            assertEquals(Query.Type.FILE, type)
        }
        with(Query.parse("${AUTHORITY}/file/${ENCODED_PATH}")) {
            assertEquals(Uri.parse(PATH), path)
            assertThrowsIllegalArgumentException { searchQuery }
            assertThrowsIllegalArgumentException { fileSize }
            assertEquals(Query.Type.FILE, type)
        }
        with(Query.parse("${AUTHORITY}/file/${ENCODED_PATH}?size=${FILE_SIZE}")) {
            assertEquals(Uri.parse(PATH), path)
            assertThrowsIllegalArgumentException { searchQuery }
            assertEquals(FILE_SIZE, fileSize)
            assertEquals(Query.Type.FILE, type)
            assertEquals(this, Query.forFile(Uri.parse(PATH), FILE_SIZE))
        }
    }

    @Test
    fun `test notification uri`() {
        assertEquals(MIME_NOTIFICATION, Query.Type.NOTIFICATION.mime)
        with(Query.parse("${AUTHORITY}/notification/${ENCODED_PATH}")) {
            assertEquals(Uri.parse(PATH), path)
            assertThrowsIllegalArgumentException { searchQuery }
            assertThrowsIllegalArgumentException { fileSize }
            assertEquals(Query.Type.NOTIFICATION, type)
            assertEquals(this, Query.forNotification(Uri.parse(PATH)))
        }
        with(Query.parse("${AUTHORITY}/notification")) {
            assertEquals(Uri.EMPTY, path)
            assertThrowsIllegalArgumentException { searchQuery }
            assertThrowsIllegalArgumentException { fileSize }
            assertEquals(Query.Type.NOTIFICATION, type)
        }
    }
}

private fun assertThrowsIllegalArgumentException(block: () -> Unit) = try {
    block()
    fail("Unexpected")
} catch (e: IllegalArgumentException) {
    e
}
