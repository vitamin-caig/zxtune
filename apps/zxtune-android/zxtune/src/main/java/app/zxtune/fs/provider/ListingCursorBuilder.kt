package app.zxtune.fs.provider

import android.database.Cursor
import android.database.MatrixCursor
import app.zxtune.fs.DefaultComparator
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsFile
import app.zxtune.fs.VfsObject
import java.util.*

internal class ListingCursorBuilder {

    private val dirs = ArrayList<VfsDir>()
    private val files = ArrayList<VfsFile>()
    private var total = 0
    private var done = 0

    fun reserve(count: Int) {
        dirs.ensureCapacity(count)
        files.ensureCapacity(count)
    }

    fun addDir(dir: VfsDir) {
        dirs.add(dir)
    }

    fun addFile(file: VfsFile) {
        files.add(file)
    }

    fun setProgress(done: Int, total: Int) {
        this.done = done
        this.total = total
    }

    fun sort(comparator: Comparator<VfsObject>? = null) = apply {
        (comparator ?: DefaultComparator.instance()).run {
            Collections.sort(dirs, this)
            Collections.sort(files, this)
        }
    }

    fun getResult(schema: SchemaSource): Cursor =
        MatrixCursor(Schema.Listing.COLUMNS, dirs.size + files.size).apply {
            schema.directories(dirs).forEach { addRow(it.serialize()) }
            schema.files(files).forEach { addRow(it.serialize()) }
        }

    val status: Cursor
        get() = if (total != 0) {
            StatusBuilder.makeProgress(done, total)
        } else {
            StatusBuilder.makeIntermediateProgress()
        }
}
