package app.zxtune.coverart

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.net.Uri
import android.util.LruCache
import androidx.annotation.MainThread
import androidx.annotation.WorkerThread
import androidx.core.util.lruCache
import app.zxtune.Logger
import app.zxtune.Releaseable
import java.util.concurrent.Executors
import java.util.concurrent.atomic.AtomicInteger

class BitmapReference(obj: Bitmap?) : Releaseable {
    private val refCount = AtomicInteger(1)
    var bitmap: Bitmap? = obj
        get() = if (refCount.get() != 0) field else null
        private set

    val usedMemorySize
        get() = bitmap?.usedMemorySize ?: 0

    override fun release() {
        if (0 == refCount.decrementAndGet()) {
            bitmap?.recycle()
            bitmap = null
        }
    }

    fun clone() = apply {
        refCount.incrementAndGet()
    }
}

class BitmapLoader(
    tag: String,
    ctx: Context,
    maxSize: Int? = null,
    maxUsedMemory: Int? = null,
    private val maxImageSize: Int? = null
) {

    init {
        require((maxSize != null) != (maxUsedMemory != null)) {
            "Specify only one type of limit"
        }
    }

    private val log = Logger("${BitmapLoader::class.java.name}[$tag]")
    private val resolver = ctx.contentResolver
    private val executor = Executors.newCachedThreadPool()

    private val cache: LruCache<Uri, BitmapReference> = maxSize?.let {
        lruCache(it, onEntryRemoved = { _, key, oldValue, _ ->
            log.d { "Remove cached bitmap for $key (${oldValue.usedMemorySize} bytes)" }
            oldValue.release()
        })
    } ?: lruCache(requireNotNull(maxUsedMemory),
        sizeOf = { _, value -> value.usedMemorySize },
        onEntryRemoved = { _, key, oldValue, _ ->
            log.d { "Remove cached bitmap for $key (${oldValue.usedMemorySize} bytes)" }
            oldValue.release()
        })

    @MainThread
    fun getCached(uri: Uri) = cache.get(uri)?.clone().also {
        log.d { "Reused cached bitmap for $uri" }
    }

    @MainThread
    fun get(uri: Uri, @WorkerThread onResult: (BitmapReference) -> Unit) {
        getCached(uri)?.let {
            onResult(it)
            return
        }
        executor.submit {
            val ref = BitmapReference(resolver.openInputStream(uri)?.use {
                val bitmap = BitmapFactory.decodeStream(it)
                if (maxImageSize != null) {
                    bitmap.fitScaledTo(maxImageSize, maxImageSize).also { res ->
                        if (res != bitmap) {
                            bitmap.recycle()
                        }
                    }
                } else {
                    bitmap
                }
            })
            log.d {
                ref.bitmap?.run {
                    "Cache bitmap for $uri ($usedMemorySize bytes, ${width}x${height})"
                } ?: "Cache no bitmap for $uri"
            }
            cache.put(uri, ref)
            onResult(ref.clone())
        }
    }
}
