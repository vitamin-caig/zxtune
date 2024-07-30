package app.zxtune.fs.provider

import android.database.Cursor
import android.database.MatrixCursor
import android.net.Uri
import android.provider.OpenableColumns
import androidx.annotation.VisibleForTesting
import app.zxtune.Logger
import app.zxtune.core.Identifier
import app.zxtune.core.jni.Api
import app.zxtune.fs.Vfs
import app.zxtune.fs.VfsFile
import java.io.IOException
import java.nio.ByteBuffer
import java.nio.channels.WritableByteChannel

internal class FileOperation @VisibleForTesting constructor(
    uri: Uri,
    private val size: Long,
    private val resolver: Resolver,
    projection: Array<String>?,
    private val readData: (VfsFile) -> ByteBuffer,
    private val api: Api,
) : AsyncQueryOperation {

    private val id = Identifier(uri)
    private val columns = projection ?: COLUMNS

    constructor(uri: Uri, size: Long, resolver: Resolver, projection: Array<String>?) : this(
        uri, size, resolver, projection, Vfs::read, Api.instance()
    )

    override fun call() = maybeResolve()?.let {
        makeResult()
    }

    private fun maybeResolve() = resolver.resolve(id.dataLocation) as? VfsFile

    override fun status(): Cursor? = null

    fun consumeContent(out: WritableByteChannel) {
        val file = maybeResolve() ?: throw IOException("Failed to resolve $id")
        val rawData = readData(file)
        if (id.subPath.isEmpty()) {
            LOG.d { "Streaming ${size}/${rawData.limit()} bytes from $id" }
            out.write(rawData.asReadOnlyBuffer().limit(size.toInt()) as ByteBuffer)
        } else {
            api.loadModuleData(rawData, id.subPath) { moduleData ->
                LOG.d { "Streaming ${size}/${moduleData.capacity()} unpacked bytes from $id" }
                out.write(moduleData.asReadOnlyBuffer().limit(size.toInt()) as ByteBuffer)
            }
        }
    }

    private fun makeResult(): Cursor {
        val cols = arrayOfNulls<String>(columns.size)
        val values = arrayOfNulls<Any>(columns.size)
        var i = 0
        for (col in columns) {
            if (OpenableColumns.DISPLAY_NAME == col) {
                cols[i] = OpenableColumns.DISPLAY_NAME
                values[i] = id.virtualFilename
                ++i
            } else if (OpenableColumns.SIZE == col) {
                cols[i] = OpenableColumns.SIZE
                values[i] = size
                ++i
            }
        }
        return MatrixCursor(cols.copyOf(i), 1).apply {
            addRow(values.copyOf(i))
        }
    }

    companion object {
        private val COLUMNS = arrayOf(
            OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE
        )
        private val LOG = Logger(FileOperation::class.java.name)
    }
}
