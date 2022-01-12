package app.zxtune.fs.provider

import android.database.Cursor
import android.database.MatrixCursor
import app.zxtune.fs.DefaultComparator
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsFile
import app.zxtune.fs.VfsObject
import java.util.*
import java.util.concurrent.CancellationException
import kotlin.collections.ArrayList

internal class ListingCursorBuilder(private val schema: SchemaSource) : VfsDir.Visitor() {

    private val dirs = ArrayList<VfsDir>()
    private val files = ArrayList<VfsFile>()
    private var total = 0
    private var done = 0

    override fun onItemsCount(count: Int) {
        dirs.ensureCapacity(count)
        files.ensureCapacity(count)
    }

    override fun onDir(dir: VfsDir) {
        dirs.add(dir)
    }

    override fun onFile(file: VfsFile) {
        files.add(file)
    }

    override fun onProgressUpdate(done: Int, total: Int) {
        checkForCancel()
        this.done = done
        this.total = total
    }

    fun getSortedResult(comparator: Comparator<VfsObject>?): Cursor {
        (comparator ?: DefaultComparator.instance()).run {
            Collections.sort(dirs, this)
            Collections.sort(files, this)
        }
        return result
    }

    val result: Cursor
        get() = MatrixCursor(Schema.Listing.COLUMNS, dirs.size + files.size).apply {
            schema.directories(dirs).forEach { addRow(it.serialize()) }
            schema.files(files).forEach { addRow(it.serialize()) }
        }

    val status: Cursor
        get() = if (total != 0) {
            StatusBuilder.makeProgress(done, total)
        } else {
            StatusBuilder.makeIntermediateProgress()
        }

    companion object {
        private fun checkForCancel() {
            if (Thread.interrupted()) {
                throw CancellationException()
            }
        }
    }
}
