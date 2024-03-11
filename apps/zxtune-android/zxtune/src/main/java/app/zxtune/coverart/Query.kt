package app.zxtune.coverart

import android.content.ContentResolver
import android.content.UriMatcher
import android.net.Uri

/*
 * ${path} is full data uri (including subpath in fragment) stored as string
 *
 * content://app.zxtune.coverart/${path} - get object properties by full path
 */

internal object Query {

    private const val AUTHORITY = "app.zxtune.coverart"

    private val uriTemplate = UriMatcher(UriMatcher.NO_MATCH).apply {
        addURI(AUTHORITY, "*", 0)
    }

    fun uriFor(uri: Uri): Uri =
        Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT).authority(AUTHORITY)
            .appendPath(uri.toString()).build()

    fun getPathFrom(uri: Uri): Uri = when (uriTemplate.match(uri)) {
        UriMatcher.NO_MATCH -> throw IllegalArgumentException("Wrong URI: $uri")
        else -> uri.pathSegments.getOrNull(0)?.let { Uri.parse(it) } ?: Uri.EMPTY
    }
}
