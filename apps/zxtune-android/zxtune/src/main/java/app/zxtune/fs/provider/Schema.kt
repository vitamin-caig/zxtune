package app.zxtune.fs.provider

import android.content.Intent
import android.database.Cursor
import android.net.Uri
import android.os.Parcel
import androidx.core.database.getBlobOrNull
import androidx.core.database.getIntOrNull
import androidx.core.database.getStringOrNull
import java.util.*
import androidx.core.net.toUri

object Schema {
    // listing:
    // int
    private const val COLUMN_TYPE = "type"

    // uri
    private const val COLUMN_URI = "uri"

    // string
    private const val COLUMN_NAME = "name"

    // string
    private const val COLUMN_DESCRIPTION = "description"

    // string - optional icon content/resource uri
    private const val COLUMN_ICON = "icon"

    // string - size for file, not null if dir has feed
    private const val COLUMN_DETAILS = "details"

    // status:
    // int
    private const val COLUMN_DONE = "done"

    // int
    private const val COLUMN_TOTAL = "total"

    // string
    private const val COLUMN_ERROR = "error"

    // serialized Intent
    private const val COLUMN_ACTION = "action"

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

    object Listing {
        val COLUMNS = arrayOf(
            COLUMN_TYPE,
            COLUMN_URI,
            COLUMN_NAME,
            COLUMN_DESCRIPTION,
            COLUMN_ICON,
            COLUMN_DETAILS,
        )

        private fun getUri(cursor: Cursor) = cursor.getString(1).toUri()
        private fun getName(cursor: Cursor) = cursor.getString(2)
        private fun getDescription(cursor: Cursor) = cursor.getString(3)
        private fun getIcon(cursor: Cursor) = cursor.getStringOrNull(4)?.toUri()

        fun parse(cursor: Cursor) = when(val type = cursor.getInt(0)) {
            TYPE_DIR -> Dir.parse(cursor)
            else -> File.parse(cursor, type)
        }

        data class Dir(
            val uri: Uri,
            val name: String,
            val description: String,
            val icon: Uri?,
            val hasFeed: Boolean
        ) : Object {

            override fun serialize() = arrayOf<Any?>(
                TYPE_DIR,
                uri.toString(),
                name,
                description,
                icon,
                if (hasFeed) "" else null,
            )

            companion object {
                fun parse(cursor: Cursor) = Dir(
                    uri = getUri(cursor),
                    name = getName(cursor),
                    description = getDescription(cursor),
                    icon = getIcon(cursor),
                    hasFeed = !cursor.isNull(5)
                )
            }
        }

        data class File(
            val uri: Uri,
            val name: String,
            val description: String,
            val icon: Uri?,
            val details: String,
            val type: Type,
        ) : Object {

            enum class Type {
                UNKNOWN,
                REMOTE,
                UNSUPPORTED,
                TRACK,
                ARCHIVE,
            }

            override fun serialize() = arrayOf<Any?>(
                TYPE_FILE + type.ordinal,
                uri.toString(),
                name,
                description,
                icon,
                details,
            )

            companion object {
                fun parse(cursor: Cursor, type: Int) = File(
                    uri = getUri(cursor),
                    name = getName(cursor),
                    description = getDescription(cursor),
                    icon = getIcon(cursor),
                    details = cursor.getString(5),
                    type = Type.entries[type - TYPE_FILE]
                )
            }
        }
    }

    object Status {
        val COLUMNS = arrayOf(COLUMN_DONE, COLUMN_TOTAL, COLUMN_ERROR)

        internal fun isStatus(cursor: Cursor) = cursor.columnNames.contentEquals(COLUMNS)

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

    object Parents {
        val COLUMNS = arrayOf(COLUMN_URI, COLUMN_NAME, COLUMN_ICON)

        data class Object(val uri: Uri, val name: String, val icon: Int?) : Schema.Object {

            override fun serialize() = arrayOf<Any?>(uri.toString(), name, icon)

            companion object {
                fun parse(cursor: Cursor) = when {
                    Status.isStatus(cursor) -> Status.parse(cursor)
                    else -> Object(
                        cursor.getString(0).toUri(),
                        cursor.getString(1),
                        cursor.getIntOrNull(2)
                    )
                }
            }
        }
    }

    object Notifications {
        val COLUMNS = arrayOf(COLUMN_DESCRIPTION, COLUMN_ACTION)

        data class Object(val message: String, val action: Intent?) : Schema.Object {
            override fun serialize() = arrayOf<Any?>(message, action?.run {
                Parcel.obtain().use {
                    writeToParcel(it, 0)
                    it.marshall()
                }
            })

            companion object {
                fun parse(cursor: Cursor) = when {
                    Status.isStatus(cursor) -> Status.parse(cursor)
                    else -> Object(
                        cursor.getString(0),
                        cursor.getBlobOrNull(1)?.run {
                            Parcel.obtain().use {
                                it.unmarshall(this, 0, size)
                                it.setDataPosition(0)
                                Intent().apply { readFromParcel(it) }
                            }
                        }
                    )
                }
            }
        }
    }
}

private fun <T> Parcel.use(cmd: (Parcel) -> T) = try {
    cmd(this)
} finally {
    recycle()
}
