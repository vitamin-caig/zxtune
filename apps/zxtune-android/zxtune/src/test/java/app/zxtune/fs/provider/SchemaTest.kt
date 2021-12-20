package app.zxtune.fs.provider

import android.database.Cursor
import android.database.MatrixCursor
import android.net.Uri
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

const val NAME1 = "Name 1"
const val NAME2 = "Name 2"
const val DESCRIPTION1 = "Description 1"
const val DESCRIPTION2 = "Description 2"
const val DETAILS1 = "Details 2"
const val DETAILS2 = "Details 2"

@RunWith(RobolectricTestRunner::class)
class SchemaTest {

    private val URI1 = Uri.parse("schema://host/path/subpath?query=value#fragment")
    private val URI2 = Uri.parse("http://example.com")

    @Test
    fun `test listing schema`() {
        testListingDir(URI1, NAME2, DESCRIPTION1, 123, false)
        testListingDir(URI2, NAME1, DESCRIPTION2, null, true)
        testListingFile(URI1, NAME2, DESCRIPTION1, DETAILS2, null, null)
        testListingFile(URI2, NAME1, DESCRIPTION2, DETAILS1, 100, true)
        testListingFile(URI1, NAME2, DESCRIPTION1, DETAILS2, 1000, false)
        testListingDelimiter()
        testListingError(Exception("Some message"))
        testListingError(Exception("Topmost", Exception("Nested")))
        testListingProgress()
    }

    @Test
    fun `test parents schema`() {
        testParent(URI1, NAME2, 123)
        testParent(URI2, NAME1, null)
    }
}

// TODO: move to class after migration
sealed interface ListingObject {
    companion object {
        fun parse(cursor: Cursor) = when {
            Schema.Status.isStatus(cursor) -> parseStatus(cursor)
            else -> parseObject(cursor)
        }

        private fun parseStatus(cursor: Cursor) = when (val err = Schema.Status.getError(cursor)) {
            null -> ListingProgress(cursor)
            else -> ListingError(err)
        }

        private fun parseObject(cursor: Cursor) = when {
            Schema.Listing.isLimiter(cursor) -> ListingDelimiter
            Schema.Listing.isDir(cursor) -> ListingDir(cursor)
            else -> ListingFile(cursor)
        }
    }
}

object ListingDelimiter : ListingObject {
    fun create(): Array<Any?> = Schema.Listing.makeLimiter()
}

sealed class ListingEntry(cursor: Cursor) : ListingObject {
    val uri: Uri = Schema.Listing.getUri(cursor)
    val name: String = Schema.Listing.getName(cursor)
    val description: String = Schema.Listing.getDescription(cursor)
}

class ListingDir(cursor: Cursor) : ListingEntry(cursor) {
    val icon = Schema.Listing.getIcon(cursor)
    val hasFeed = Schema.Listing.hasFeed(cursor)

    companion object {
        fun create(
            uri: Uri,
            name: String,
            description: String,
            icon: Int?,
            hasFeed: Boolean
        ): Array<Any?> = Schema.Listing.makeDirectory(uri, name, description, icon, hasFeed)
    }
}

class ListingFile(cursor: Cursor) : ListingEntry(cursor) {
    val details: String = Schema.Listing.getDetails(cursor)
    val tracks = Schema.Listing.getTracks(cursor)
    val isCached = Schema.Listing.isCached(cursor)

    companion object {
        fun create(
            uri: Uri,
            name: String,
            description: String,
            details: String,
            tracks: Int?,
            isCached: Boolean?
        ): Array<Any?> = Schema.Listing.makeFile(uri, name, description, details, tracks, isCached)
    }
}

class ListingError(val error: String) : ListingObject {
    companion object {
        fun create(e: Exception): Array<Any?> = Schema.Status.makeError(e)
    }
}

class ListingProgress(cursor: Cursor) : ListingObject {
    val done = Schema.Status.getDone(cursor)
    val total = Schema.Status.getTotal(cursor)

    companion object {
        fun createIntermediate(): Array<Any?> = Schema.Status.makeIntermediateProgress()
        fun create(done: Int): Array<Any?> = Schema.Status.makeProgress(done)
        fun create(done: Int, total: Int): Array<Any?> = Schema.Status.makeProgress(done, total)
    }
}

private fun testListingDir(
    uri: Uri,
    name: String,
    description: String,
    icon: Int?,
    hasFeed: Boolean
) = MatrixCursor(Schema.Listing.COLUMNS).apply {
    addRow(ListingDir.create(uri, name, description, icon, hasFeed))
}.use { cursor ->
    cursor.moveToFirst()
    (ListingObject.parse(cursor) as ListingDir).let { dir ->
        assertEquals(uri, dir.uri)
        assertEquals(name, dir.name)
        assertEquals(description, dir.description)
        assertEquals(icon, dir.icon)
        assertEquals(hasFeed, dir.hasFeed)
    }
}

private fun testListingFile(
    uri: Uri,
    name: String,
    description: String,
    details: String,
    tracks: Int?,
    isCached: Boolean?
) = MatrixCursor(Schema.Listing.COLUMNS).apply {
    addRow(ListingFile.create(uri, name, description, details, tracks, isCached))
}.use { cursor ->
    cursor.moveToFirst()
    (ListingObject.parse(cursor) as ListingFile).let { file ->
        assertEquals(uri, file.uri)
        assertEquals(name, file.name)
        assertEquals(description, file.description)
        assertEquals(details, file.details)
        assertEquals(tracks, file.tracks)
        assertEquals(isCached, file.isCached)
    }
}

private fun testListingDelimiter() = MatrixCursor(Schema.Listing.COLUMNS).apply {
    addRow(ListingDelimiter.create())
}.use { cursor ->
    cursor.moveToFirst()
    assertTrue(ListingObject.parse(cursor) is ListingDelimiter)
}

private fun testListingError(e: Exception) = MatrixCursor(Schema.Status.COLUMNS).apply {
    addRow(ListingError.create(e))
}.use { cursor ->
    cursor.moveToFirst()
    assertEquals(
        e.cause?.message ?: e.message,
        (ListingObject.parse(cursor) as ListingError).error
    )
}

private fun testListingProgress() = MatrixCursor(Schema.Status.COLUMNS).apply {
    addRow(ListingProgress.createIntermediate())
    addRow(ListingProgress.create(1))
    addRow(ListingProgress.create(2, 3))
}.use { cursor ->
    cursor.moveToNext()
    (ListingObject.parse(cursor) as ListingProgress).let {
        assertEquals(-1, it.done)
        assertEquals(100, it.total)
    }
    cursor.moveToNext()
    (ListingObject.parse(cursor) as ListingProgress).let {
        assertEquals(1, it.done)
        assertEquals(100, it.total)
    }
    cursor.moveToNext()
    (ListingObject.parse(cursor) as ListingProgress).let {
        assertEquals(2, it.done)
        assertEquals(3, it.total)
    }
}

class ParentObject private constructor(cursor: Cursor) {
    val uri: Uri = Schema.Parents.getUri(cursor)
    val name: String = Schema.Parents.getName(cursor)
    val icon = Schema.Parents.getIcon(cursor)

    companion object {
        fun create(uri: Uri, name: String, icon: Int?): Array<Any?> =
            Schema.Parents.make(uri, name, icon)

        fun parse(cursor: Cursor) = ParentObject(cursor)
    }
}

private fun testParent(
    uri: Uri,
    name: String,
    icon: Int?
) = MatrixCursor(Schema.Parents.COLUMNS).apply {
    addRow(ParentObject.create(uri, name, icon))
}.use { cursor ->
    cursor.moveToFirst()
    ParentObject.parse(cursor).let { parent ->
        assertEquals(uri, parent.uri)
        assertEquals(name, parent.name)
        assertEquals(icon, parent.icon)
    }
}
