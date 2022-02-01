package app.zxtune.fs.provider

import android.database.Cursor
import android.database.MatrixCursor
import android.net.Uri
import android.os.ParcelFileDescriptor
import android.provider.OpenableColumns
import androidx.annotation.VisibleForTesting
import app.zxtune.fs.Vfs
import app.zxtune.fs.VfsFile
import java.io.File
import java.io.IOException

internal class FileOperation(
    private val uri: Uri,
    private val resolver: Resolver,
    projection: Array<String>?
) : AsyncQueryOperation {

    private val columns = projection ?: COLUMNS

    override fun call() = maybeResolve()?.let { file ->
        maybeGetFile(file)?.let {
            makeResult(file, it)
        }
    }

    private fun maybeResolve() = resolver.resolve(uri) as? VfsFile

    private fun maybeGetFile(file: VfsFile) = Vfs.getCacheOrFile(file)?.takeIf { it.isFile }

    override fun status(): Cursor? = null

    fun openFile(mode: String): ParcelFileDescriptor? = ParcelFileDescriptor.open(
        openFileInternal(mode),
        ParcelFileDescriptor.MODE_READ_ONLY
    )

    @VisibleForTesting
    fun openFileInternal(mode: String): File {
        require("r" == mode) { "Invalid mode: $mode" }
        val file = maybeResolve() ?: throw IOException("Failed to resolve $uri")
        return maybeGetFile(file) ?: throw IOException("Failed to get file content of $uri")
    }

    // as in FileProvider
    private fun makeResult(file: VfsFile, content: File): Cursor {
        val cols = arrayOfNulls<String>(columns.size)
        val values = arrayOfNulls<Any>(columns.size)
        var i = 0
        for (col in columns) {
            if (OpenableColumns.DISPLAY_NAME == col) {
                cols[i] = OpenableColumns.DISPLAY_NAME
                values[i] = file.name
                ++i
            } else if (OpenableColumns.SIZE == col) {
                cols[i] = OpenableColumns.SIZE
                values[i] = content.length()
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
    }
}
