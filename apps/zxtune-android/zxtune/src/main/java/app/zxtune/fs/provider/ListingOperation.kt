package app.zxtune.fs.provider

import android.net.Uri
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsFile
import app.zxtune.fs.comparator

internal class ListingOperation(
    private val uri: Uri,
    private val resolver: Resolver,
    private val schema: SchemaSource,
    private val callback: AsyncQueryOperation.Callback,
) : AsyncQueryOperation {

    private val builder = ListingCursorBuilder()

    override fun call() = maybeResolve()?.let { dir ->
        dir.enumerate(object : VfsDir.Visitor() {
            override fun onItemsCount(count: Int) = builder.reserve(count)
            override fun onProgressUpdate(done: Int, total: Int) {
                callback.checkForCancel()
                builder.setProgress(done, total)
                callback.onStatusChanged()
            }

            override fun onDir(dir: VfsDir) = builder.addDir(dir)
            override fun onFile(file: VfsFile) = builder.addFile(file)
        })
        callback.checkForCancel()
        builder.sort(dir.comparator).getResult(schema)
    }

    private fun maybeResolve() = resolver.resolve(uri) as? VfsDir

    override fun status() = builder.status
}
