package app.zxtune.fs.provider

import android.database.Cursor
import android.net.Uri

internal class ParentsOperation(
    private val uri: Uri,
    private val resolver: Resolver,
    private val schema: SchemaSource
) : AsyncQueryOperation {
    override fun call() = ParentsCursorBuilder.makeParents(resolver.resolve(uri), schema)

    override fun status(): Cursor? = null
}
