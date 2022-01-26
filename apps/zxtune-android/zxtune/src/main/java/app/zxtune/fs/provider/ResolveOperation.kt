package app.zxtune.fs.provider

import android.database.MatrixCursor
import android.net.Uri
import androidx.core.os.OperationCanceledException
import app.zxtune.Logger

internal class ResolveOperation(
    private val uri: Uri,
    private val resolver: Resolver,
    private val schema: SchemaSource
) : AsyncQueryOperation {

    private var done = -1
    private var total = 100

    override fun call() = maybeResolve()?.let { obj ->
        MatrixCursor(Schema.Listing.COLUMNS, 1).apply {
            schema.resolved(obj)?.let {
                addRow(it.serialize())
            }
        }
    }

    private fun maybeResolve() = resolver.resolve(uri) { done: Int, total: Int ->
        checkForCancel()
        this.done = done
        this.total = total
    }

    private fun checkForCancel() {
        if (Thread.interrupted()) {
            LOG.d { "Canceled resolving for $uri" }
            throw OperationCanceledException()
        }
    }

    override fun status() = StatusBuilder.makeProgress(done, total)

    companion object {
        private val LOG = Logger(ResolveOperation::class.java.name)
    }
}
