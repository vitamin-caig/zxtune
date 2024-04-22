package app.zxtune.coverart

import android.graphics.Bitmap
import android.graphics.Bitmap.CompressFormat
import android.os.Build
import androidx.core.graphics.BitmapCompat
import java.io.ByteArrayOutputStream

fun <R> Bitmap.use(block: (Bitmap) -> R) = try {
    block(this)
} finally {
    recycle()
}

val Bitmap.usedMemorySize: Int
    get() = BitmapCompat.getAllocationByteCount(this)

fun Bitmap.compress(format: CompressFormat, quality: Int): ByteArray =
    ByteArrayOutputStream(usedMemorySize.takeHighestOneBit() / 16).use { out ->
        compress(format, quality, out)
        out.toByteArray()
    }

fun Bitmap.toJpeg(quality: Int = 90) = compress(CompressFormat.JPEG, quality)
fun Bitmap.toPng() = compress(CompressFormat.PNG, 100)

fun Bitmap.fitScaledTo(maxWidth: Int, maxHeight: Int): Bitmap {
    // scale = minOf(maxWidth / width, maxHeight / height)
    // dstWidth = width * scale
    // dstHeight = height * scale
    // then scale *= area, dstWidth /= area, dstHeight /= area
    val scale = minOf(maxWidth * height, maxHeight * width)
    val dstWidth = scale / height
    val dstHeight = scale / width
    return BitmapCompat.createScaledBitmap(this, dstWidth, dstHeight, null, true)
}
