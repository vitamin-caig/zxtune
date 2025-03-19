/**
 * @file
 * @brief VFS provider query helper
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.provider

import android.content.ContentResolver
import android.content.UriMatcher
import android.net.Uri

/*
 * ${path} is full data uri (including subpath in fragment) stored as string
 *
 * content://app.zxtune.vfs/resolve/${path} - get object properties by full path
 * content://app.zxtune.vfs/listing/${path} - get directory content by full path
 * content://app.zxtune.vfs/parents/${path} - get object parents chain
 * content://app.zxtune.vfs/search/${path}?query=${query} - start search
 * content://app.zxtune.vfs/file/${path}?size=${size} - get information/content of track file
 * content://app.zxtune.vfs/notification/${path} - get path-related notification
 */
internal object Query {
    enum class Type(val path: String, val mime: String) {
        RESOLVE("resolve", MIME_ITEM),

        LISTING("listing", MIME_ITEMS_SET),

        PARENTS("parents", MIME_SIMPLE_ITEMS_SET),

        SEARCH("search", MIME_ITEMS_SET),

        FILE("file", MIME_ITEM),

        NOTIFICATION("notification", MIME_NOTIFICATION),
    }

    private const val AUTHORITY = "app.zxtune.vfs"
    private const val QUERY_PARAM = "query"
    private const val SIZE_PARAM = "size"
    private const val ITEM_SUBTYPE = "vnd.$AUTHORITY.item"
    private const val SIMPLE_ITEM_SUBTYPE = "vnd.$AUTHORITY.simple_item"
    private const val NOTIFICATION_SUBTYPE = "vnd.$AUTHORITY.notification"
    private const val MIME_ITEM = "${ContentResolver.CURSOR_ITEM_BASE_TYPE}/$ITEM_SUBTYPE"
    private const val MIME_ITEMS_SET = "${ContentResolver.CURSOR_DIR_BASE_TYPE}/$ITEM_SUBTYPE"
    private const val MIME_SIMPLE_ITEMS_SET =
        "${ContentResolver.CURSOR_DIR_BASE_TYPE}/$SIMPLE_ITEM_SUBTYPE"
    private const val MIME_NOTIFICATION =
        "${ContentResolver.CURSOR_ITEM_BASE_TYPE}/$NOTIFICATION_SUBTYPE"
    private val uriTemplate = UriMatcher(UriMatcher.NO_MATCH).apply {
        Type.entries.forEach {
            // Empty path for empty root url
            addURI(AUTHORITY, it.path, it.ordinal)
            addURI(AUTHORITY, "${it.path}/*", it.ordinal)
        }
    }

    fun getUriType(uri: Uri) = Type.entries.getOrNull(uriTemplate.match(uri))

    fun getPathFrom(uri: Uri): Uri = when (getUriType(uri)) {
        Type.RESOLVE, Type.LISTING, Type.PARENTS, Type.SEARCH, Type.FILE, Type.NOTIFICATION -> uri.pathSegments.getOrNull(
            1
        )?.let { Uri.parse(it) } ?: Uri.EMPTY

        else -> throw IllegalArgumentException("Wrong URI: $uri")
    }

    fun getQueryFrom(uri: Uri) =
        uri.takeIf { getUriType(uri) == Type.SEARCH }?.getQueryParameter(QUERY_PARAM)
            ?: throw IllegalArgumentException("Wrong search URI: $uri")

    fun getSizeFrom(uri: Uri) =
        uri.takeIf { getUriType(uri) == Type.FILE }?.getQueryParameter(SIZE_PARAM)?.toLongOrNull()
            ?: throw IllegalArgumentException("Wrong file URI: $uri")

    fun resolveUriFor(uri: Uri): Uri = makeUri(Type.RESOLVE, uri).build()

    fun listingUriFor(uri: Uri): Uri = makeUri(Type.LISTING, uri).build()

    fun parentsUriFor(uri: Uri): Uri = makeUri(Type.PARENTS, uri).build()

    fun searchUriFor(uri: Uri, query: String): Uri =
        makeUri(Type.SEARCH, uri).appendQueryParameter(QUERY_PARAM, query).build()

    fun fileUriFor(uri: Uri, size: Long): Uri =
        makeUri(Type.FILE, uri).appendQueryParameter(SIZE_PARAM, size.toString()).build()

    fun notificationUriFor(uri: Uri): Uri = makeUri(Type.NOTIFICATION, uri).build()

    private fun makeUri(type: Type, uri: Uri) =
        Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT).authority(AUTHORITY)
            .encodedPath(type.path).appendPath(uri.toString())
}
