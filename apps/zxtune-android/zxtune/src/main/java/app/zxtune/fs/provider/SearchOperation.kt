package app.zxtune.fs.provider

import android.database.Cursor
import android.database.MatrixCursor
import android.net.Uri
import androidx.core.os.OperationCanceledException
import app.zxtune.Logger
import app.zxtune.fs.*
import java.io.InterruptedIOException
import java.util.concurrent.atomic.AtomicReference

internal class SearchOperation(
    private val uri: Uri,
    private val resolver: Resolver,
    private val schema: SchemaSource,
    query: String
) : AsyncQueryOperation {
    private val query = query.lowercase()
    private val result = AtomicReference<ArrayList<VfsFile>?>()

    override fun call(): Cursor {
        result.set(ArrayList())
        maybeResolve()?.let { search(it) }
        return convert(result.getAndSet(null)!!).apply {
            addRow(Schema.Listing.Delimiter.serialize())
        }
    }

    private fun maybeResolve() = resolver.resolve(uri) as? VfsDir

    private fun search(dir: VfsDir) = search(dir) { obj ->
        checkForCancel()
        synchronized(result) { result.get()!!.add(obj) }
    }

    private fun search(dir: VfsDir, visitor: VfsExtensions.SearchEngine.Visitor) =
        dir.searchEngine?.find(query, visitor) ?: iterate(dir) {
            if (match(it.name) || match(it.description)) {
                visitor.onFile(it)
            } else {
                checkForCancel()
            }
        }

    private fun iterate(dir: VfsDir, visitor: VfsExtensions.SearchEngine.Visitor) =
        VfsIterator(dir) { e ->
            if (e is InterruptedIOException) {
                throwCanceled()
            } else {
                LOG.w(e) { "Ignore I/O error" }
            }
        }.run {
            while (isValid) {
                visitor.onFile(file)
                next()
            }
        }

    private fun match(txt: String) = txt.lowercase().contains(query)

    private fun checkForCancel() {
        if (Thread.interrupted()) {
            throwCanceled()
        }
    }

    private fun throwCanceled() {
        LOG.d { "Canceled search for $query at $uri" }
        throw OperationCanceledException()
    }

    override fun status() = intermediateResult()?.let { convert(it) }

    private fun intermediateResult() = synchronized(result) {
        result.get()?.let {
            result.set(ArrayList())
            it
        }
    }

    private fun convert(found: ArrayList<VfsFile>) =
        MatrixCursor(Schema.Listing.COLUMNS, found.size + 1).apply {
            schema.files(found).forEach { addRow(it.serialize()) }
        }

    companion object {
        private val LOG = Logger(SearchOperation::class.java.name)
    }
}
