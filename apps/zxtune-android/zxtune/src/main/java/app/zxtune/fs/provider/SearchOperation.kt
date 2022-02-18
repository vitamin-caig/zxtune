package app.zxtune.fs.provider

import android.database.Cursor
import android.database.MatrixCursor
import android.net.Uri
import app.zxtune.Logger
import app.zxtune.fs.*
import java.util.concurrent.atomic.AtomicReference

internal class SearchOperation(
    private val uri: Uri,
    private val resolver: Resolver,
    private val schema: SchemaSource,
    private val callback: AsyncQueryOperation.Callback,
    query: String
) : AsyncQueryOperation {
    private val query = query.lowercase()
    private val result = AtomicReference<ArrayList<VfsFile>?>()

    override fun call(): Cursor {
        result.set(ArrayList())
        maybeResolve()?.let { search(it) }
        return convert(result.getAndSet(null)!!)
    }

    private fun maybeResolve() = resolver.resolve(uri) as? VfsDir

    private fun search(dir: VfsDir) = search(dir) { obj ->
        synchronized(result) { result.get()!!.add(obj) }
        callback.onStatusChanged()
    }

    private fun search(dir: VfsDir, visitor: VfsExtensions.SearchEngine.Visitor) =
        dir.searchEngine?.find(query, visitor) ?: iterate(dir) {
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

    private fun match(txt: String) = txt.lowercase().contains(query)

    override fun status() = intermediateResult()?.let { convert(it) }

    private fun intermediateResult() = synchronized(result) {
        result.get()?.let {
            result.set(ArrayList())
            it
        }
    }

    private fun convert(found: ArrayList<VfsFile>) =
        MatrixCursor(Schema.Listing.COLUMNS, found.size).apply {
            schema.files(found).forEach { addRow(it.serialize()) }
        }

    companion object {
        private val LOG = Logger(SearchOperation::class.java.name)
    }
}
