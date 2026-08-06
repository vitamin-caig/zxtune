/**
 * @file
 * @brief VFS provider query helper
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.provider

import android.content.ContentResolver
import android.content.UriMatcher
import android.net.Uri
import androidx.core.net.toUri
import app.zxtune.BuildConfig

/*
 * ${path} is full data uri (including subpath in fragment) stored as string
 *
 * content://app.zxtune.vfs/resolve/${path} - get object properties by full path and all parents
 * content://app.zxtune.vfs/listing/${path} - get directory content by full path
 * content://app.zxtune.vfs/feed/${path} - VfsExtensions.FEED values
 * content://app.zxtune.vfs/search/${path}?query=${query} - start search
 * content://app.zxtune.vfs/file/${path}?size=${size} - get information/content of track file
 * content://app.zxtune.vfs/notification/${path} - get path-related notification
 */
internal class Query private constructor(val providerUri: Uri, val type: Type?) {
    enum class Type(val path: String, val mime: String) {
        RESOLVE("resolve", MIME_ITEMS_SET),

        LISTING("listing", MIME_ITEMS_SET),

        FEED("feed", MIME_ITEMS_SET),

        SEARCH("search", MIME_ITEMS_SET),

        FILE("file", MIME_ITEM),

        NOTIFICATION("notification", MIME_NOTIFICATION),
    }

    companion object {
        private const val AUTHORITY = "${BuildConfig.APPLICATION_ID}.vfs"
        private const val QUERY_PARAM = "query"
        private const val SIZE_PARAM = "size"
        private const val ITEM_SUBTYPE = "vnd.$AUTHORITY.item"
        private const val NOTIFICATION_SUBTYPE = "vnd.$AUTHORITY.notification"
        private const val MIME_ITEM = "${ContentResolver.CURSOR_ITEM_BASE_TYPE}/$ITEM_SUBTYPE"
        private const val MIME_ITEMS_SET = "${ContentResolver.CURSOR_DIR_BASE_TYPE}/$ITEM_SUBTYPE"
        private const val MIME_NOTIFICATION =
            "${ContentResolver.CURSOR_ITEM_BASE_TYPE}/$NOTIFICATION_SUBTYPE"
        private val uriTemplate = UriMatcher(UriMatcher.NO_MATCH).apply {
            Type.entries.forEach {
                // Empty path for empty root url
                addURI(AUTHORITY, it.path, it.ordinal)
                addURI(AUTHORITY, "${it.path}/*", it.ordinal)
            }
        }

        fun parse(uri: Uri) = Query(uri, Type.entries.getOrNull(uriTemplate.match(uri)))
        fun parse(uri: String) = parse(uri.toUri())

        fun forResolve(uri: Uri) = makeSimple(Type.RESOLVE, uri)

        fun forListing(uri: Uri) = makeSimple(Type.LISTING, uri)

        fun forFeed(uri: Uri) = makeSimple(Type.FEED, uri)

        fun forSearch(uri: Uri, query: String) =
            makeComplex(Type.SEARCH, uri) { appendQueryParameter(QUERY_PARAM, query) }

        fun forFile(uri: Uri, size: Long) =
            makeComplex(Type.FILE, uri) { appendQueryParameter(SIZE_PARAM, size.toString()) }

        fun forNotification(uri: Uri) = makeSimple(Type.NOTIFICATION, uri)

        private fun makeUri(type: Type, uri: Uri) =
            Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT).authority(AUTHORITY)
                .encodedPath(type.path).appendPath(uri.toString())

        private fun makeSimple(type: Type, uri: Uri) = Query(makeUri(type, uri).build(), type)

        private fun makeComplex(type: Type, uri: Uri, patch: Uri.Builder.() -> Unit) =
            Query(makeUri(type, uri).apply(patch).build(), type)
    }

    val path: Uri
        get() = when (type) {
            Type.RESOLVE, Type.LISTING, Type.FEED, Type.SEARCH, Type.FILE, Type.NOTIFICATION -> providerUri.pathSegments.getOrNull(
                1
            )?.toUri() ?: Uri.EMPTY

            else -> throw IllegalArgumentException("Wrong URI: $providerUri")
        }

    val searchQuery
        get() = providerUri.takeIf { type == Type.SEARCH }?.getQueryParameter(QUERY_PARAM)
            ?: throw IllegalArgumentException("Wrong search URI: $providerUri")

    val fileSize
        get() = providerUri.takeIf { type == Type.FILE }?.getQueryParameter(SIZE_PARAM)
            ?.toLongOrNull() ?: throw IllegalArgumentException("Wrong file URI: $providerUri")
}
