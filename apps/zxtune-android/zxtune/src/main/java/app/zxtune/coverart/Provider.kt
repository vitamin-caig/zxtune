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
import android.support.v4.media.MediaMetadataCompat
import androidx.annotation.DrawableRes
import androidx.annotation.VisibleForTesting
import androidx.appcompat.content.res.AppCompatResources
import androidx.core.graphics.drawable.toBitmap
import androidx.core.net.toUri
import app.zxtune.Logger
import app.zxtune.MainApplication
import app.zxtune.core.Identifier
import app.zxtune.fs.Vfs
import app.zxtune.fs.VfsFile
import app.zxtune.fs.VfsObject
import app.zxtune.fs.icon
import java.io.FileOutputStream
import java.io.OutputStream
import java.nio.ByteBuffer
import java.nio.channels.Channels

class Provider @VisibleForTesting internal constructor(
    private val resolve: (Uri) -> VfsObject?,
) : ContentProvider() {
    constructor() : this(::resolve)

    private val svc = CoverartService.get()
    private val source = CoversSource(1_048_576, this::loadImage)

    private fun loadImage(case: Query.Case, id: String, size: Point?) = when (case) {
        Query.Case.RAW -> id.toLongOrNull()?.let { fetchBlob(it) }
        Query.Case.RES -> id.toIntOrNull()?.let { renderIcon(it, size ?: DEFAULT_ICON_SIZE) }
        Query.Case.IMAGE -> loadImageFile(id.toUri(), size)
    }

    private fun fetchBlob(id: Long) = svc.getBlob(id)?.toByteBuffer()

    private fun renderIcon(@DrawableRes res: Int, size: Point) =
        AppCompatResources.getDrawable(requireNotNull(context), res)?.toBitmap(
            size.x, size.y, Bitmap.Config.RGB_565
        )?.use {
            it.toPng()
        }?.toByteBuffer()

    private fun loadImageFile(uri: Uri, size: Point?): ByteBuffer? {
        svc.imageFor(uri)?.run {
            pic?.data?.let {
                LOG.d { "Load blob image for $uri" }
                return it.toByteBuffer()
            }
            url?.let {
                require(it != uri)
                return loadImageFile(it, size)
            }
            return null
        }
        val entry = resolve(uri) ?: return null
        (entry as? VfsFile)?.let { file ->
            LOG.d { "Load image $uri" }
            return runCatching { Vfs.openStream(file) }.getOrNull()?.use {
                svc.addPicture(Identifier(uri), it)?.data
            }?.toByteBuffer()
        }
        return null
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

    override fun call(method: String, arg: String?, extras: Bundle?) = when (method) {
        METHOD_GET_MEDIA_URIS -> arg?.let { getMediaUris(Identifier.parse(it)) }
        else -> null
    }

    private fun getMediaUris(id: Identifier) = resolve(id.dataLocation)?.let { obj ->
        Bundle().apply {
            svc.coverArtUriOf(id)?.let {
                putParcelable(
                    MediaMetadataCompat.METADATA_KEY_ART_URI, it
                )
            } ?: svc.albumArtUriOf(id, obj)?.let {
                putParcelable(
                    MediaMetadataCompat.METADATA_KEY_ALBUM_ART_URI, it
                )
            }
            resolve(Uri.Builder().scheme(id.dataLocation.scheme).build())?.icon?.let {
                putParcelable(
                    MediaMetadataCompat.METADATA_KEY_DISPLAY_ICON_URI,
                    Query.uriFor(Query.Case.RES, it.toString())
                )
            }
        }
    }

    override fun openFile(uri: Uri, mode: String) = openPipeHelper(
        uri, "image/*", null, null, ImageDataWriter(source)
    )

    override fun openTypedAssetFile(
        uri: Uri, mimeTypeFilter: String, opts: Bundle?, signal: CancellationSignal?
    ) = openPipeHelper(
        uri, mimeTypeFilter, opts, sizeFrom(opts), ImageDataWriter(source)
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
        internal const val METHOD_GET_MEDIA_URIS = "get_media_uris"

        private fun sizeFrom(opts: Bundle?) = if (Build.VERSION.SDK_INT >= 21) {
            opts?.getParcelable<Point>(ContentResolver.EXTRA_SIZE)
        } else {
            null
        }

        private fun resolve(uri: Uri) = runCatching {
            Vfs.resolve(uri)
        }.getOrNull()
    }
}

private fun ByteArray.toByteBuffer() = ByteBuffer.wrap(this)
