package app.zxtune.fs.provider

import android.database.Cursor
import android.net.Uri
import androidx.core.database.getIntOrNull
import androidx.core.database.getStringOrNull
import java.util.*

internal object Schema {
    // int
    private const val COLUMN_TYPE = "type"

    // uri
    private const val COLUMN_URI = "uri"

    // string
    private const val COLUMN_NAME = "name"

    // string
    private const val COLUMN_DESCRIPTION = "description"

    // int - icon res or tracks count
    private const val COLUMN_ICON = "icon"

    // string - size for file, not null if dir has feed
    private const val COLUMN_DETAILS = "details"

    // bool
    private const val COLUMN_CACHED = "cached"

    // int
    private const val COLUMN_DONE = "done"

    // int
    private const val COLUMN_TOTAL = "total"

    // string
    private const val COLUMN_ERROR = "error"

    private const val TYPE_DIR = 0
    private const val TYPE_FILE = 1

    sealed interface Object {

        fun serialize(): Array<Any?>

        companion object {
            fun parse(cursor: Cursor) = when {
                Status.isStatus(cursor) -> Status.parse(cursor)
                else -> Listing.parse(cursor)
            }
        }
    }

    internal object Listing {
        val COLUMNS = arrayOf(
            COLUMN_TYPE,
            COLUMN_URI,
            COLUMN_NAME,
            COLUMN_DESCRIPTION,
            COLUMN_ICON,
            COLUMN_DETAILS,
            COLUMN_CACHED
        )

        private fun getUri(cursor: Cursor) = Uri.parse(cursor.getString(1))
        private fun getName(cursor: Cursor) = cursor.getString(2)
        private fun getDescription(cursor: Cursor) = cursor.getString(3)

        fun parse(cursor: Cursor) = when {
            cursor.isNull(1) -> Delimiter
            cursor.getInt(0) == TYPE_DIR -> Dir.parse(cursor)
            else -> File.parse(cursor)
        }

        object Delimiter : Object {
            override fun serialize() = arrayOfNulls<Any>(COLUMNS.size)
        }

        data class Dir(
            val uri: Uri,
            val name: String,
            val description: String,
            val icon: Int?,
            val hasFeed: Boolean
        ) : Object {

            override fun serialize() = arrayOf<Any?>(
                TYPE_DIR,
                uri.toString(),
                name,
                description,
                icon,
                if (hasFeed) "" else null,
                null
            )

            companion object {
                fun parse(cursor: Cursor) = Dir(
                    uri = getUri(cursor),
                    name = getName(cursor),
                    description = getDescription(cursor),
                    icon = cursor.getIntOrNull(4),
                    hasFeed = !cursor.isNull(5)
                )
            }
        }

        data class File(
            val uri: Uri,
            val name: String,
            val description: String,
            val details: String,
            val tracks: Int?,
            val isCached: Boolean?
        ) : Object {

            override fun serialize() = arrayOf<Any?>(
                TYPE_FILE,
                uri.toString(),
                name,
                description,
                tracks,
                details,
                isCached.toInt()
            )

            companion object {
                fun parse(cursor: Cursor) = File(
                    uri = getUri(cursor),
                    name = getName(cursor),
                    description = getDescription(cursor),
                    tracks = cursor.getIntOrNull(4),
                    details = cursor.getString(5),
                    isCached = cursor.getIntOrNull(6).toBoolean()
                )
            }
        }
    }

    internal object Status {
        val COLUMNS = arrayOf(COLUMN_DONE, COLUMN_TOTAL, COLUMN_ERROR)

        internal fun isStatus(cursor: Cursor) = Arrays.equals(cursor.columnNames, COLUMNS)

        fun parse(cursor: Cursor) = when (val err = cursor.getStringOrNull(2)) {
            null -> Progress.parse(cursor)
            else -> Error(err)
        }

        data class Error(val error: String) : Object {

            constructor(e: Throwable) : this(e.cause?.message ?: e.message ?: e.toString())

            override fun serialize(): Array<Any?> = arrayOf(0, 0, error)
        }

        data class Progress(val done: Int, val total: Int = 100) : Object {

            override fun serialize() = arrayOf<Any?>(done, total, null)

            companion object {
                fun createIntermediate() = Progress(-1)

                fun parse(cursor: Cursor) = Progress(cursor.getInt(0), cursor.getInt(1))
            }
        }
    }

    internal object Parents {
        val COLUMNS = arrayOf(COLUMN_URI, COLUMN_NAME, COLUMN_ICON)

        data class Object(val uri: Uri, val name: String, val icon: Int?) : Schema.Object {

            override fun serialize() = arrayOf<Any?>(uri.toString(), name, icon)

            companion object {
                fun parse(cursor: Cursor) = when {
                    Status.isStatus(cursor) -> Status.parse(cursor)
                    else -> Object(
                        Uri.parse(cursor.getString(0)),
                        cursor.getString(1),
                        cursor.getIntOrNull(2)
                    )
                }
            }
        }
    }
}

private fun Int?.toBoolean() = this?.let { it != 0 }

private fun Boolean?.toInt() = this?.let { if (it) 1 else 0 }
