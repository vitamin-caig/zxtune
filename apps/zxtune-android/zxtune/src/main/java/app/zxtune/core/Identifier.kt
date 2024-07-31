package app.zxtune.core

import android.net.Uri

private const val SUBPATH_DELIMITER = '/'
private const val TRACK_INDEX_PREFIX = '#'
private const val PACKED_DATA_PREFIX = "+un"

/**
 * Identifier is not fully compatible with playlists from desktop version of zxtune
 * <p>
 * E.g. first track in ay file is stored as test.ay?#1 without hash sign encoding
 */
class Identifier private constructor(builder: Uri.Builder) {

    val fullLocation: Uri = builder.build()

    val dataLocation: Uri = builder.fragment(null).build()

    val subPath
        get() = fullLocation.fragment.orEmpty()

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

    /**
     * @returns optional track index for multitrack modules
     */
    val trackIndex: Int?
        get() = subPath.substringAfterLast("$SUBPATH_DELIMITER$TRACK_INDEX_PREFIX", "")
            .toIntOrNull()

    /**
     * @returns user-friendly display name
     */
    val displayFilename: String
        get() {
            val filename = dataLocation.lastPathSegment.orEmpty()
            return findNestedFilename(acceptTrackIndex = true)?.let { nestedName ->
                "$filename > $nestedName"
            } ?: filename
        }

    /**
     * @returns filename suitable for saving
     */
    val virtualFilename: String
        get() = findNestedFilename(acceptTrackIndex = false)
            ?: dataLocation.lastPathSegment.orEmpty()

    /**
     * @returns last suitable path component of subpath
     */
    private fun findNestedFilename(acceptTrackIndex: Boolean = false): String? {
        val path = subPath
        if (path.isEmpty()) {
            return null
        }
        var result: String? = null
        var start = 0
        do {
            val next = path.indexOf(SUBPATH_DELIMITER, start)
            val part = if (next == -1) path.substring(start) else path.substring(start, next)
            if (!part.startsWith(PACKED_DATA_PREFIX) && (!part.startsWith(TRACK_INDEX_PREFIX) || acceptTrackIndex)) {
                result = part
            }
            start = next + 1
        } while (start != 0)
        return result
    }

    override fun toString() = fullLocation.toString()

    override fun hashCode() = fullLocation.hashCode()

    override fun equals(other: Any?) = (other as? Identifier)?.fullLocation == fullLocation

    companion object {
        @JvmField
        val EMPTY = Identifier(Uri.EMPTY)

        @JvmStatic
        fun parse(str: String?) = str?.let { Identifier(Uri.parse(it)) } ?: EMPTY
    }
}
