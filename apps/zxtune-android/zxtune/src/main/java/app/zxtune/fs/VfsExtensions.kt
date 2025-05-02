/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs

import android.content.ContentResolver
import android.content.Intent
import android.net.Uri
import androidx.annotation.DrawableRes
import java.io.File
import java.io.FileDescriptor
import java.io.IOException
import java.io.InputStream

object VfsExtensions {
    // Remote autoplay URI, string
    const val SHARE_URL = "SHARE_URL"
    const val SEARCH_ENGINE = "SEARCH_ENGINE"

    // Comparator<VfsObject>
    const val COMPARATOR = "COMPARATOR"

    // java.util.Iterator<VfsFile>
    const val FEED = "FEED"

    // String
    const val CACHE_PATH = "CACHE_PATH"

    // File
    const val FILE = "FILE"

    // Uri[]
    const val DOWNLOAD_URIS = "DOWNLOAD_URIS"

    // @DrawableRes
    const val ICON = "ICON"

    // Uri of vfs object
    const val ICON_URI = "ICON_URI"

    // Intent
    const val PERMISSION_QUERY_INTENT = "PERMISSION_QUERY_INTENT"

    // InputStream
    const val INPUT_STREAM = "INPUT_STREAM"

    // FileDescriptor
    const val FILE_DESCRIPTOR = "FILE_DESCRIPTOR"

    // String
    const val COVER_ART_URI = "COVER_ART_URI"

    // Separate interface for fast searching
    interface SearchEngine {
        fun interface Visitor {
            fun onFile(file: VfsFile)
        }

        @Throws(IOException::class)
        fun find(query: String, visitor: Visitor)
    }
}

val VfsObject.shareUrl
    get() = getExtension(VfsExtensions.SHARE_URL) as? String

val VfsDir.searchEngine
    get() = getExtension(VfsExtensions.SEARCH_ENGINE) as? VfsExtensions.SearchEngine

val VfsDir.comparator
    @Suppress("UNCHECKED_CAST") get() = getExtension(VfsExtensions.COMPARATOR) as? Comparator<VfsObject>

val VfsDir.feed
    @Suppress("UNCHECKED_CAST") get() = getExtension(VfsExtensions.FEED) as? Iterator<VfsFile>

val VfsFile.file
    get() = getExtension(VfsExtensions.FILE) as? File

@get:DrawableRes
val VfsObject.icon
    get() = getExtension(VfsExtensions.ICON) as? Int

val VfsObject.iconUri
    get() = icon?.let {
        Uri.Builder().scheme(ContentResolver.SCHEME_ANDROID_RESOURCE).appendPath(it.toString())
            .build()
    } ?: getExtension(VfsExtensions.ICON_URI) as? Uri

val VfsObject.permissionQueryIntent
    get() = getExtension(VfsExtensions.PERMISSION_QUERY_INTENT) as? Intent

val VfsFile.inputStream
    get() = getExtension(VfsExtensions.INPUT_STREAM) as? InputStream

val VfsFile.fileDescriptor
    get() = getExtension(VfsExtensions.FILE_DESCRIPTOR) as? FileDescriptor

val VfsObject.coverArtUri
    get() = getExtension(VfsExtensions.COVER_ART_URI) as? Uri
