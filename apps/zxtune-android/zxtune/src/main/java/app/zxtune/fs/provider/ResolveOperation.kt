package app.zxtune.fs.provider

import android.database.MatrixCursor
import app.zxtune.fs.VfsObject

internal class ResolveOperation(
    private val query: Query,
    private val resolver: Resolver,
    private val schema: SchemaSource,
    private val callback: AsyncQueryOperation.Callback,
) : AsyncQueryOperation {

    private var done = -1
    private var total = 100

    override fun call() = maybeResolve()?.let { obj ->
        MatrixCursor(Schema.Content.COLUMNS, 10).apply {
            var current: VfsObject? = obj
            while (true) {
                current?.let {
                    addRow(schema.resolved(it).serialize())
                    current = it.parent
                } ?: break
            }
        }
    }

    private fun maybeResolve() = resolver.resolve(query.path) { done: Int, total: Int ->
        callback.checkForCancel()
        this.done = done
        this.total = total
        callback.onStatusChanged()
    }

    override fun status() = StatusBuilder.makeProgress(done, total)
}
