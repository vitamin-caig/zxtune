package app.zxtune.fs.provider

import android.content.Intent
import android.database.MatrixCursor
import android.net.Uri
import androidx.core.net.toUri
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
    private val ICON_URI1 = "icon://uri/of".toUri()

    @Test
    fun `test listing schema`() {
        testListingDir(URI1, NAME2, DESCRIPTION1, ICON_URI1, false)
        testListingDir(URI2, NAME1, DESCRIPTION2, null, true)
        testListingFile(URI1, NAME2, DESCRIPTION1, null, DETAILS2, Schema.Listing.File.Type.REMOTE)
        testListingFile(URI2, NAME1, DESCRIPTION2, null, DETAILS1, Schema.Listing.File.Type.UNKNOWN)
        testListingFile(
            URI1, NAME2, DESCRIPTION1, ICON_URI1, DETAILS2, Schema.Listing.File.Type.TRACK
        )
        testListingFile(
            URI2, NAME1, DESCRIPTION2, ICON_URI1, DETAILS1, Schema.Listing.File.Type.ARCHIVE
        )
        testListingFile(
            URI1, NAME2, DESCRIPTION1, ICON_URI1, DETAILS2, Schema.Listing.File.Type.UNSUPPORTED
        )
        testListingError(Exception("Some message"))
        testListingError(Exception("Topmost", Exception("Nested")))
        testListingProgress()
    }

    @Test
    fun `test parents schema`() {
        testParent(URI1, NAME2, 123)
        testParent(URI2, NAME1, null)
    }

    @Test
    fun `test notification`() {
        testNotification("", null)
        testNotification("Only message", null)
        testNotification("Intent action", Intent("action"))
        testNotification("Intent action and uri", Intent("action", Uri.parse("schema://host")))
        testNotification("Intent extra", Intent().apply { putExtra("extra", 123) })
    }
}

private fun testListingDir(
    uri: Uri, name: String, description: String, icon: Uri?, hasFeed: Boolean
) = MatrixCursor(Schema.Listing.COLUMNS).apply {
    addRow(Schema.Listing.Dir(uri, name, description, icon, hasFeed).serialize())
}.use { cursor ->
    cursor.moveToFirst()
    (Schema.Object.parse(cursor) as Schema.Listing.Dir).let { dir ->
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
    icon: Uri?,
    details: String,
    type: Schema.Listing.File.Type,
) = MatrixCursor(Schema.Listing.COLUMNS).apply {
    addRow(Schema.Listing.File(uri, name, description, icon, details, type).serialize())
}.use { cursor ->
    cursor.moveToFirst()
    (Schema.Object.parse(cursor) as Schema.Listing.File).let { file ->
        assertEquals(uri, file.uri)
        assertEquals(name, file.name)
        assertEquals(description, file.description)
        assertEquals(icon, file.icon)
        assertEquals(details, file.details)
        assertEquals(type, file.type)
    }
}

private fun testListingError(e: Exception) = MatrixCursor(Schema.Status.COLUMNS).apply {
    addRow(Schema.Status.Error(e).serialize())
}.use { cursor ->
    cursor.moveToFirst()
    assertEquals(
        e.cause?.message ?: e.message, (Schema.Object.parse(cursor) as Schema.Status.Error).error
    )
}

private fun testListingProgress() = MatrixCursor(Schema.Status.COLUMNS).apply {
    addRow(Schema.Status.Progress.createIntermediate().serialize())
    addRow(Schema.Status.Progress(1).serialize())
    addRow(Schema.Status.Progress(2, 3).serialize())
}.use { cursor ->
    cursor.moveToNext()
    (Schema.Object.parse(cursor) as Schema.Status.Progress).let {
        assertEquals(-1, it.done)
        assertEquals(100, it.total)
    }
    cursor.moveToNext()
    (Schema.Object.parse(cursor) as Schema.Status.Progress).let {
        assertEquals(1, it.done)
        assertEquals(100, it.total)
    }
    cursor.moveToNext()
    (Schema.Object.parse(cursor) as Schema.Status.Progress).let {
        assertEquals(2, it.done)
        assertEquals(3, it.total)
    }
}

private fun testParent(
    uri: Uri, name: String, icon: Int?
) = MatrixCursor(Schema.Parents.COLUMNS).apply {
    addRow(Schema.Parents.Object(uri, name, icon).serialize())
}.use { cursor ->
    cursor.moveToFirst()
    (Schema.Parents.Object.parse(cursor) as Schema.Parents.Object).let { parent ->
        assertEquals(uri, parent.uri)
        assertEquals(name, parent.name)
        assertEquals(icon, parent.icon)
    }
}

private fun testNotification(
    message: String, action: Intent?
) = MatrixCursor(Schema.Notifications.COLUMNS).apply {
    addRow(Schema.Notifications.Object(message, action).serialize())
}.use { cursor ->
    cursor.moveToFirst()
    (Schema.Notifications.Object.parse(cursor) as Schema.Notifications.Object).let { notification ->
        assertEquals(message, notification.message)
        assertTrue(action?.filterEquals(notification.action) ?: (notification.action == null))
    }
}
