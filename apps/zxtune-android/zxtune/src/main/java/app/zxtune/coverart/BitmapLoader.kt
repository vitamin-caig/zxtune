package app.zxtune.coverart

import android.content.Context
import android.graphics.Bitmap
import android.graphics.BitmapFactory
import android.net.Uri
import android.util.LruCache
import android.widget.ImageView
import androidx.annotation.DrawableRes
import androidx.core.util.lruCache
import androidx.tracing.traceAsync
import app.zxtune.Logger
import app.zxtune.utils.ifNotNulls
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

class BitmapReference(val bitmap: Bitmap?) {
    val usedMemorySize
        get() = bitmap?.usedMemorySize ?: 0
}

class BitmapLoader(
    private val tag: String,
    ctx: Context,
    maxSize: Int? = null,
    maxUsedMemory: Int? = null,
    private val maxImageSize: Int? = null,
    private val readDispatcher: CoroutineDispatcher = Dispatchers.IO,
    private val resizeDispatcher: CoroutineDispatcher = Dispatchers.Default,
) {

    init {
        require((maxSize != null) != (maxUsedMemory != null)) {
            "Specify only one type of limit"
        }
    }

    private val log = Logger("${BitmapLoader::class.java.name}[$tag]")
    private val resolver = ctx.contentResolver

    private val cache: LruCache<Uri, BitmapReference> = maxSize?.let {
        lruCache(it, onEntryRemoved = { _, key, oldValue, _ ->
            log.d { "Remove cached bitmap for $key (${oldValue.usedMemorySize} bytes)" }
        })
    } ?: lruCache(requireNotNull(maxUsedMemory),
        sizeOf = { _, value -> value.usedMemorySize },
        onEntryRemoved = { _, key, oldValue, _ ->
            log.d { "Remove cached bitmap for $key (${oldValue.usedMemorySize} bytes)" }
        })

    fun getCached(uri: Uri) = cache.get(uri)?.also {
        log.d { "Reused cached bitmap for $uri (${it.usedMemorySize} bytes)" }
    }

    suspend fun load(uri: Uri) = traceAsync("load $tag", tag.hashCode()) {
        loadAndResize(uri)
    }.also {
        log.d {
            it.bitmap?.run {
                "Cache bitmap for $uri ($usedMemorySize bytes, ${width}x${height})"
            } ?: "Cache no bitmap for $uri"
        }
        cache.put(uri, it)
    }

    private suspend fun loadAndResize(uri: Uri): BitmapReference {
        val bitmap = loadBitmap(uri)
        return BitmapReference(ifNotNulls(bitmap, maxImageSize) { it, size ->
            resize(it, size)
        } ?: bitmap)
    }

    private suspend fun loadBitmap(uri: Uri) = withContext(readDispatcher) {
        loadBitmapFromUri(uri)
    }

    // TODO: use ImageDecoder.Source, @see ImageView.getDrawableFromUri
    private fun loadBitmapFromUri(uri: Uri) = runCatching {
        resolver.openInputStream(uri)?.use {
            BitmapFactory.decodeStream(it)
        }
    }.onFailure { log.w(it) { "Failed to load bitmap" } }.getOrNull()

    private suspend fun resize(bitmap: Bitmap, size: Int) = withContext(resizeDispatcher) {
        bitmap.fitScaledTo(size, size).also { res ->
            if (res != bitmap) {
                bitmap.recycle()
            }
        }
    }
}

sealed interface ImageSource {
    fun applyTo(img: ImageView)
}

class BitmapSource(val src: Bitmap) : ImageSource {
    override fun applyTo(img: ImageView) = img.setImageBitmap(src)
}

class ResourceSource(@DrawableRes val src: Int) : ImageSource {
    override fun applyTo(img: ImageView) = img.setImageResource(src)
}
