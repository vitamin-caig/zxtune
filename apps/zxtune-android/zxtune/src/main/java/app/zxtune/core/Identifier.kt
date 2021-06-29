package app.zxtune.core

import android.net.Uri

/**
 * Identifier is not fully compatible with playlists from desktop version of zxtune
 * <p>
 * E.g. first track in ay file is stored as test.ay?#1 without hash sign encoding
 */
class Identifier private constructor(builder: Uri.Builder) {

    val fullLocation: Uri = builder.build()

    val dataLocation: Uri = builder.fragment(null).build()

    val subPath: String
        get() = fullLocation.fragment.orEmpty()

    val displayFilename: String
        get() {
            val filename = dataLocation.lastPathSegment.orEmpty()
            val nestedName = subPath
            return if (nestedName.isEmpty()) {
                filename
            } else {
                "$filename > ${nestedName.substringAfterLast(SUBPATH_DELIMITER)}"
            }
        }

    constructor(location: Uri) : this(
        if (location.path.isNullOrEmpty()) {
            Uri.EMPTY.buildUpon()
        } else {
            location.buildUpon()
        }
    )

    constructor(location: Uri, subPath: String?) : this(
        if (location.path.isNullOrEmpty()) {
            Uri.EMPTY.buildUpon()
        } else {
            location.buildUpon().fragment(subPath)
        }
    )

    override fun toString() = fullLocation.toString()

    override fun hashCode() = fullLocation.hashCode()

    override fun equals(other: Any?) = (other as? Identifier)?.fullLocation == fullLocation

    companion object {
        const val SUBPATH_DELIMITER = '/'

        @JvmField
        val EMPTY = Identifier(Uri.EMPTY)

        @JvmStatic
        fun parse(str: String?) = str?.let { Identifier(Uri.parse(it)) } ?: EMPTY
    }
}
