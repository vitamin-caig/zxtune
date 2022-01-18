package app.zxtune.fs.provider

import android.net.Uri
import androidx.annotation.VisibleForTesting
import androidx.collection.LruCache
import app.zxtune.Logger
import app.zxtune.fs.VfsArchive
import app.zxtune.fs.VfsObject
import app.zxtune.utils.ProgressCallback

internal class CachingResolver(cacheSize: Int = 10) : Resolver {

    companion object {
        private val LOG = Logger(CachingResolver::class.java.name)
    }

    @VisibleForTesting
    val cache = object : LruCache<Uri, VfsObject>(cacheSize) {
        override fun entryRemoved(
            evicted: Boolean,
            key: Uri,
            oldValue: VfsObject,
            newValue: VfsObject?
        ) {
            if (evicted) {
                LOG.d { "Remove cache for $key" }
            }
        }
    }

    override fun resolve(uri: Uri) =
        cache.get(uri) ?: VfsArchive.resolve(uri)?.also { cache.put(uri, it) }

    override fun resolve(uri: Uri, cb: ProgressCallback) =
        cache.get(uri) ?: VfsArchive.resolveForced(uri, cb)?.also { cache.put(uri, it) }
}
