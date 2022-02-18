package app.zxtune.fs.provider

import android.database.MatrixCursor
import android.net.Uri

internal class ResolveOperation(
    private val uri: Uri,
    private val resolver: Resolver,
    private val schema: SchemaSource,
    private val callback: AsyncQueryOperation.Callback,
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
        callback.checkForCancel()
        this.done = done
        this.total = total
        callback.onStatusChanged()
    }

    override fun status() = StatusBuilder.makeProgress(done, total)
}
