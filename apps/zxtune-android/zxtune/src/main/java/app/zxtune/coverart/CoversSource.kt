package app.zxtune.coverart

import android.graphics.Point
import android.net.Uri
import androidx.core.util.lruCache
import java.nio.ByteBuffer
import java.util.concurrent.atomic.AtomicReference
import java.util.concurrent.locks.ReentrantReadWriteLock
import kotlin.concurrent.read
import kotlin.concurrent.write

class CoversSource(maxCacheSize: Int, private val factory: (Uri, size: Point?) -> ByteBuffer?) {

    private val storage = lruCache<Uri, AtomicReference<ByteBuffer?>>(maxCacheSize,
        sizeOf = { _, value -> value.get()?.capacity() ?: 0 },
        onEntryRemoved = { _, uri, _, _ -> Provider.LOG.d { "Remove cached cover for $uri" } })
    private val lock = ReentrantReadWriteLock()

    fun query(uri: Uri, size: Point? = null): ByteBuffer? = lock.read {
        storage.get(uri)?.let {
            Provider.LOG.d { "Reuse cached cover for $uri" }
            return it.get()
        }
        lock.write { // upgrade lock, safe to nest
            // double check cache
            storage.get(uri)?.let {
                Provider.LOG.d { "Reuse cached cover for $uri" }
                return it.get()
            }
            factory(uri, size).also {
                Provider.LOG.d {
                    "Create and cache cover for $uri (${it?.capacity() ?: 0} bytes, ${size ?: "<any size>"})"
                }
                storage.put(uri, AtomicReference(it))
            }
        }
    }
}
