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
            return subPath.extractFilename { !it.isPackedDataIdentifier }?.let { nestedName ->
                "$filename > $nestedName"
            } ?: filename
        }

    val virtualFilename
        get() = subPath.extractFilename { !it.isPackedDataIdentifier && !it.isTrackIndexIdentifier }
            ?: dataLocation.lastPathSegment.orEmpty()

    val trackIndex: Int?
        get() = subPath.substringAfterLast("${SUBPATH_DELIMITER}${TRACK_INDEX_PREFIX}", "")
            .toIntOrNull()

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
        const val TRACK_INDEX_PREFIX = '#'

        @JvmField
        val EMPTY = Identifier(Uri.EMPTY)

        @JvmStatic
        fun parse(str: String?) = str?.let { Identifier(Uri.parse(it)) } ?: EMPTY
    }
}

private val CharSequence.isPackedDataIdentifier
    get() = startsWith("+un")

private val CharSequence.isTrackIndexIdentifier
    get() = startsWith(Identifier.TRACK_INDEX_PREFIX)

private fun CharSequence.splitAsPath() =
    when (val lastDelimiter = lastIndexOf(Identifier.SUBPATH_DELIMITER)) {
        -1 -> "" to this
        else -> subSequence(0, lastDelimiter) to subSequence(lastDelimiter + 1, length)
    }

private fun CharSequence.extractFilename(filter: (CharSequence) -> Boolean): CharSequence? =
    splitAsPath().run {
        when {
            second.isEmpty() -> null
            filter(second) -> second
            first.isEmpty() -> null
            else -> first.extractFilename(filter)
        }
    }
