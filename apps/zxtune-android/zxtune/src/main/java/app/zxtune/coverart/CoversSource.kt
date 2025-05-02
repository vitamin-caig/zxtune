package app.zxtune.coverart

import android.graphics.Point
import android.net.Uri
import androidx.core.util.lruCache
import java.nio.ByteBuffer
import java.util.concurrent.atomic.AtomicReference
import java.util.concurrent.locks.ReentrantReadWriteLock
import kotlin.concurrent.read
import kotlin.concurrent.write

internal class CoversSource(
    maxCacheSize: Int, private val factory: (Query.Case, String, size: Point?) -> ByteBuffer?
) {
    private val storage = lruCache<Uri, AtomicReference<ByteBuffer?>>(
        maxCacheSize,
        sizeOf = { _, value -> value.get()?.capacity() ?: 16 },
        onEntryRemoved = { _, uri, _, _ -> Provider.LOG.d { "Remove cached cover for $uri" } })
    private val lock = ReentrantReadWriteLock()

    fun query(uri: Uri, size: Point? = null): ByteBuffer? = lock.read {
        storage.get(uri)?.let {
            return it.get()
        }
        val case = Query.getCase(uri) ?: return null
        val id = Query.idFrom(uri) ?: return null
        Provider.LOG.d { "Query $case for $id" }
        val res = factory(case, id, size)
        if (res == null || case == Query.Case.IMAGE || case == Query.Case.RES) {
            Provider.LOG.d {
                "Cache $uri (${res?.capacity() ?: 0} bytes, ${size ?: "<any size>"})"
            }
            lock.write { // upgrade lock, safe to nest
                storage.put(uri, AtomicReference(res))
            }
        }
        return res
    }
}
