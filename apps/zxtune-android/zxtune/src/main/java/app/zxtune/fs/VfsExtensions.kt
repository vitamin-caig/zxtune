/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs

import androidx.annotation.DrawableRes
import java.io.IOException

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

    // Separate interface for fast searching
    interface SearchEngine {
        fun interface Visitor {
            fun onFile(file: VfsFile)
        }

        @Throws(IOException::class)
        fun find(query: String, visitor: Visitor)
    }
}

val VfsDir.searchEngine
    get() = getExtension(VfsExtensions.SEARCH_ENGINE) as? VfsExtensions.SearchEngine

val VfsDir.comparator
    @Suppress("UNCHECKED_CAST")
    get() = getExtension(VfsExtensions.COMPARATOR) as? Comparator<VfsObject>

val VfsDir.feed
    @Suppress("UNCHECKED_CAST")
    get() = getExtension(VfsExtensions.FEED) as? Iterator<VfsFile>

@get:DrawableRes
val VfsObject.icon
    get() = getExtension(VfsExtensions.ICON) as? Int
