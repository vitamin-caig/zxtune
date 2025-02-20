package app.zxtune.coverart

import android.content.ContentProvider
import android.content.ContentResolver
import android.content.ContentValues
import android.content.res.AssetFileDescriptor
import android.graphics.Bitmap
import android.graphics.Point
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.CancellationSignal
import android.os.ParcelFileDescriptor
import androidx.annotation.DrawableRes
import androidx.annotation.VisibleForTesting
import androidx.appcompat.content.res.AppCompatResources
import androidx.core.graphics.drawable.toBitmap
import app.zxtune.Logger
import app.zxtune.MainApplication
import app.zxtune.core.Identifier
import app.zxtune.fs.Vfs
import app.zxtune.fs.VfsFile
import app.zxtune.fs.icon
import app.zxtune.fs.provider.CachingResolver
import app.zxtune.fs.provider.Resolver
import java.io.FileOutputStream
import java.io.OutputStream
import java.nio.ByteBuffer
import java.nio.channels.Channels

class Provider @VisibleForTesting internal constructor(
    private val resolver: Resolver,
) : ContentProvider() {
    constructor() : this(CachingResolver(cacheSize = 100))

    private val source = CoversSource(1_048_576, this::loadImage)

    private fun loadImage(uri: Uri, size: Point?): ByteBuffer? {
        val svc = CoverartService.get()
        val img = svc.imageFor(uri) ?: return null
        (img as? ByteArray)?.let {
            LOG.d { "Load blob image for $uri" }
            return ByteBuffer.wrap(it)
        }
        val fsUri = img as Uri
        if (fsUri != uri) {
            loadImage(fsUri, size)?.let {
                return it
            }
        }
        // no callback to disable archive analysis
        val entry = resolver.resolve(fsUri) ?: return null
        entry.icon?.let { icon ->
            LOG.d { "Render icon for $fsUri" }
            return loadIcon(icon, size ?: DEFAULT_ICON_SIZE)?.let {
                ByteBuffer.wrap(it)
            }
        }
        (entry as? VfsFile)?.let { file ->
            LOG.d { "Load image $fsUri" }
            return runCatching { Vfs.openStream(file) }.getOrNull()?.use {
                svc.addPicture(Identifier(fsUri), it)
            }?.let {
                ByteBuffer.wrap(it)
            }
        }
        return null
    }

    private fun loadIcon(@DrawableRes res: Int, size: Point) =
        AppCompatResources.getDrawable(requireNotNull(context), res)?.toBitmap(
            size.x, size.y, Bitmap.Config.RGB_565
        )?.use {
            it.toPng()
        }

    override fun onCreate() = context?.run {
        MainApplication.initialize(applicationContext)
        true
    } ?: false

    override fun query(
        uri: Uri,
        projection: Array<String>?,
        selection: String?,
        selectionArgs: Array<String>?,
        sortOrder: String?
    ) = TODO("Not implemented")

    override fun getType(uri: Uri) = null

    override fun insert(uri: Uri, values: ContentValues?) = TODO("Not implemented")

    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<String>?) =
        TODO("Not implemented")

    override fun update(
        uri: Uri, values: ContentValues?, selection: String?, selectionArgs: Array<String>?
    ) = TODO("Not yet implemented")

    override fun openFile(uri: Uri, mode: String) = openPipeHelper(
        Query.getPathFrom(uri), "image/*", null, null, ImageDataWriter(source)
    )

    override fun openTypedAssetFile(
        uri: Uri, mimeTypeFilter: String, opts: Bundle?, signal: CancellationSignal?
    ) = openPipeHelper(
        Query.getPathFrom(uri), mimeTypeFilter, opts, sizeFrom(opts), ImageDataWriter(source)
    ).let {
        AssetFileDescriptor(it, 0, -1)
    }

    private class ImageDataWriter(
        private val src: CoversSource,
    ) : PipeDataWriter<Point?> {

        override fun writeDataToPipe(
            output: ParcelFileDescriptor, uri: Uri, mimeType: String, opts: Bundle?, size: Point?
        ) = FileOutputStream(output.fileDescriptor).use {
            writeData(uri, size, it)
            Unit
        }

        private fun writeData(uri: Uri, size: Point?, out: OutputStream) = runCatching {
            src.query(uri, size)?.let {
                Channels.newChannel(out).write(it.asReadOnlyBuffer())
            }
        }.onFailure { err ->
            LOG.w(err) { "Failed to send coverart" }
        }
    }


    companion object {
        internal val LOG = Logger(Provider::class.java.name)
        private val DEFAULT_ICON_SIZE = Point(256, 256)

        private fun sizeFrom(opts: Bundle?) = if (Build.VERSION.SDK_INT >= 21) {
            opts?.getParcelable<Point>(ContentResolver.EXTRA_SIZE)
        } else {
            null
        }
    }
}
