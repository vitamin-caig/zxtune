package app.zxtune.fs.provider

import android.net.Uri
import androidx.annotation.VisibleForTesting
import androidx.collection.lruCache
import app.zxtune.Logger
import app.zxtune.fs.VfsArchive
import app.zxtune.fs.VfsObject
import app.zxtune.utils.ProgressCallback

internal class CachingResolver(cacheSize: Int = 10) : Resolver {

    companion object {
        private val LOG = Logger(CachingResolver::class.java.name)
    }

    @VisibleForTesting
    val cache = lruCache<Uri, VfsObject>(cacheSize, onEntryRemoved = { evicted, key, _, _ ->
        if (evicted) {
            LOG.d { "Remove cache for $key" }
        }
    })

    override fun resolve(uri: Uri) =
        cache.get(uri) ?: runCatching { VfsArchive.resolve(uri) }.getOrNull()
            ?.also { cache.put(uri, it) }

    override fun resolve(uri: Uri, cb: ProgressCallback) =
        cache.get(uri) ?: runCatching { VfsArchive.resolveForced(uri, cb) }.getOrNull()
            ?.also { cache.put(uri, it) }
}
