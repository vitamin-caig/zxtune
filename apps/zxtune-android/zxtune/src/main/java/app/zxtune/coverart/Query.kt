package app.zxtune.coverart

import android.content.ContentResolver
import android.content.UriMatcher
import android.net.Uri
import app.zxtune.BuildConfig

/*
 * ${path} is full data uri (including subpath in fragment) stored as string
 *
 * content://app.zxtune.coverart/${path} - get object properties by full path
 */

internal object Query {

    private const val AUTHORITY = "${BuildConfig.APPLICATION_ID}.coverart"
    private val baseUriBuilder
        get() = Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT).authority(AUTHORITY)

    private val uriTemplate = UriMatcher(UriMatcher.NO_MATCH).apply {
        addURI(AUTHORITY, "*", 0)
    }

    val rootUri: Uri = baseUriBuilder.build()

    fun uriFor(uri: Uri): Uri = baseUriBuilder.appendPath(uri.toString()).build()

    fun getPathFrom(uri: Uri): Uri = when (uriTemplate.match(uri)) {
        UriMatcher.NO_MATCH -> throw IllegalArgumentException("Wrong URI: $uri")
        else -> uri.pathSegments.getOrNull(0)?.let { Uri.parse(it) } ?: Uri.EMPTY
    }
}
