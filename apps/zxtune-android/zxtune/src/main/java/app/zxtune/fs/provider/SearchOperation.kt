package app.zxtune.fs.provider

import android.database.Cursor
import android.database.MatrixCursor
import app.zxtune.Logger
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsExtensions
import app.zxtune.fs.VfsFile
import app.zxtune.fs.VfsIterator
import app.zxtune.fs.searchEngine
import java.util.concurrent.atomic.AtomicReference

internal class SearchOperation(
    private val query: Query,
    private val resolver: Resolver,
    private val schema: SchemaSource,
    private val callback: AsyncQueryOperation.Callback,
) : AsyncQueryOperation {
    private val searchQuery = query.searchQuery.lowercase()
    private val result = AtomicReference<ArrayList<VfsFile>?>()

    override fun call(): Cursor {
        result.set(ArrayList())
        maybeResolve()?.let { search(it) }
        return convert(result.getAndSet(null)!!)
    }

    private fun maybeResolve() = resolver.resolve(query.path) as? VfsDir

    private fun search(dir: VfsDir) = search(dir) { obj ->
        synchronized(result) { result.get()!!.add(obj) }
        callback.onStatusChanged()
    }

    private fun search(dir: VfsDir, visitor: VfsExtensions.SearchEngine.Visitor) =
        dir.searchEngine?.find(searchQuery, visitor) ?: iterate(dir) {
            callback.checkForCancel()
            if (match(it.name) || match(it.description)) {
                visitor.onFile(it)
            }
        }

    private fun iterate(dir: VfsDir, visitor: VfsExtensions.SearchEngine.Visitor) =
        VfsIterator(dir) { e ->
            callback.checkForCancel()
            LOG.w(e) { "Ignore I/O error" }
        }.run {
            while (isValid) {
                visitor.onFile(file)
                next()
            }
        }

    private fun match(txt: String) = txt.lowercase().contains(searchQuery)

    override fun status() = intermediateResult()?.let { convert(it) }

    private fun intermediateResult() = synchronized(result) {
        result.get()?.let {
            result.set(ArrayList())
            it
        }
    }

    private fun convert(found: ArrayList<VfsFile>) =
        MatrixCursor(Schema.Content.COLUMNS, found.size).apply {
            schema.files(found).forEach { addRow(it.serialize()) }
        }

    companion object {
        private val LOG = Logger(SearchOperation::class.java.name)
    }
}
