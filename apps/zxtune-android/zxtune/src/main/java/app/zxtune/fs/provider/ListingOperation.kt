package app.zxtune.fs.provider

import android.net.Uri
import app.zxtune.fs.VfsDir
import app.zxtune.fs.comparator

internal class ListingOperation(
    private val uri: Uri,
    private val resolver: Resolver,
    schema: SchemaSource
) : AsyncQueryOperation {

    private val builder = ListingCursorBuilder(schema)

    override fun call() = maybeResolve()?.let { dir ->
        dir.enumerate(builder)
        builder.getSortedResult(dir.comparator)
    }

    private fun maybeResolve() = resolver.resolve(uri) as? VfsDir

    override fun status() = builder.status
}
