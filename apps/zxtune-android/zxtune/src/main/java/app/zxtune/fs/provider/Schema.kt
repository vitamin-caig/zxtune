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
        @JvmField
        val COLUMNS = arrayOf(
            COLUMN_TYPE,
            COLUMN_URI,
            COLUMN_NAME,
            COLUMN_DESCRIPTION,
            COLUMN_ICON,
            COLUMN_DETAILS,
            COLUMN_CACHED
        )

        @Deprecated("Outdated", ReplaceWith("Dir.create"))
        @JvmStatic
        fun makeDirectory(
            uri: Uri,
            name: String,
            description: String,
            icon: Int?,
            hasFeed: Boolean
        ): Array<Any?> = arrayOf(
            TYPE_DIR,
            uri.toString(),
            name,
            description,
            icon,
            if (hasFeed) "" else null,
            null
        )

        @Deprecated("Outdated", ReplaceWith("File.create"))
        @JvmStatic
        fun makeFile(
            uri: Uri,
            name: String,
            description: String,
            details: String,
            tracks: Int?,
            isCached: Boolean?
        ): Array<Any?> =
            arrayOf(TYPE_FILE, uri.toString(), name, description, tracks, details, isCached.toInt())

        @Deprecated("Outdated", ReplaceWith("Delimiter.create"))
        @JvmStatic
        fun makeLimiter() = arrayOfNulls<Any>(COLUMNS.size)

        // For searches
        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun isLimiter(cursor: Cursor) = cursor.isNull(1)

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun isDir(cursor: Cursor) = cursor.getInt(0) == TYPE_DIR

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun getUri(cursor: Cursor): Uri = Uri.parse(cursor.getString(1))

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun getName(cursor: Cursor): String = cursor.getString(2)

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun getDescription(cursor: Cursor): String = cursor.getString(3)

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun getIcon(cursor: Cursor): Int? = cursor.getIntOrNull(4)

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun getDetails(cursor: Cursor): String = cursor.getString(5)

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun hasFeed(cursor: Cursor) = !cursor.isNull(5)

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun getTracks(cursor: Cursor): Int? = cursor.getIntOrNull(4)

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun isCached(cursor: Cursor) = cursor.getIntOrNull(6).toBoolean()

        fun parse(cursor: Cursor) = when {
            isLimiter(cursor) -> Delimiter
            isDir(cursor) -> Dir.parse(cursor)
            else -> File.parse(cursor)
        }

        object Delimiter : Object {
            override fun serialize() = makeLimiter()
        }

        data class Dir(
            val uri: Uri,
            val name: String,
            val description: String,
            val icon: Int?,
            val hasFeed: Boolean
        ) : Object {

            override fun serialize() = makeDirectory(uri, name, description, icon, hasFeed)

            companion object {
                fun parse(cursor: Cursor) = Dir(
                    getUri(cursor),
                    getName(cursor),
                    getDescription(cursor),
                    getIcon(cursor),
                    hasFeed(cursor)
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

            override fun serialize() = makeFile(uri, name, description, details, tracks, isCached)

            companion object {
                fun parse(cursor: Cursor) = File(
                    getUri(cursor),
                    getName(cursor),
                    getDescription(cursor),
                    getDetails(cursor),
                    getTracks(cursor),
                    isCached(cursor)
                )
            }
        }
    }

    internal object Status {
        @JvmField
        val COLUMNS = arrayOf(COLUMN_DONE, COLUMN_TOTAL, COLUMN_ERROR)

        @Deprecated("Outdated", ReplaceWith("Progress.createIntermediate"))
        @JvmStatic
        fun makeIntermediateProgress() = makeProgress(-1)

        @Deprecated("Outdated", ReplaceWith("Progress.create"))
        @JvmOverloads
        @JvmStatic
        fun makeProgress(done: Int, total: Int = 100) = arrayOf<Any?>(done, total, null)

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun isStatus(cursor: Cursor) = Arrays.equals(cursor.columnNames, COLUMNS)

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun getDone(cursor: Cursor) = cursor.getInt(0)

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun getTotal(cursor: Cursor) = cursor.getInt(1)

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun getError(cursor: Cursor): String? = cursor.getStringOrNull(2)

        fun parse(cursor: Cursor) = when (val err = getError(cursor)) {
            null -> Progress.parse(cursor)
            else -> Error(err)
        }

        data class Error(val error: String) : Object {

            constructor(e: Throwable) : this(e.cause?.message ?: e.message ?: e.toString())

            override fun serialize(): Array<Any?> = arrayOf(0, 0, error)
        }

        data class Progress(val done: Int, val total: Int = 100) : Object {

            override fun serialize() = makeProgress(done, total)

            companion object {
                fun createIntermediate() = Progress(-1)

                fun parse(cursor: Cursor) = Progress(getDone(cursor), getTotal(cursor))
            }
        }
    }

    internal object Parents {
        @JvmField
        val COLUMNS = arrayOf(COLUMN_URI, COLUMN_NAME, COLUMN_ICON)

        @Deprecated("Outdated", ReplaceWith("Object.create"))
        @JvmStatic
        fun make(uri: Uri, name: String, icon: Int?): Array<Any?> =
            arrayOf(uri.toString(), name, icon)

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun getUri(cursor: Cursor): Uri = Uri.parse(cursor.getString(0))

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun getName(cursor: Cursor): String = cursor.getString(1)

        @Deprecated("Outdated", ReplaceWith("Object.parse"))
        @JvmStatic
        fun getIcon(cursor: Cursor): Int? = cursor.getIntOrNull(2)

        data class Object(val uri: Uri, val name: String, val icon: Int?) {

            fun serialize() = make(uri, name, icon)

            companion object {
                fun parse(cursor: Cursor) = Object(getUri(cursor), getName(cursor), getIcon(cursor))
            }
        }
    }
}

private fun Int?.toBoolean() = this?.let { it != 0 }

private fun Boolean?.toInt() = this?.let { if (it) 1 else 0 }
