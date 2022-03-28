package app.zxtune.fs

import android.content.Context
import android.net.Uri
import app.zxtune.R
import app.zxtune.fs.aminet.Path
import app.zxtune.fs.aminet.RemoteCatalog
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.fs.httpdir.Catalog
import app.zxtune.fs.httpdir.HttpRootBase

internal class VfsRootAminet private constructor(
    parent: VfsObject,
    private val context: Context,
    private val remote: RemoteCatalog
) : HttpRootBase(parent, Catalog.create(context, remote, "aminet"), Path.create()), VfsRoot {

    constructor(parent: VfsObject, context: Context, http: MultisourceHttpProvider) : this(
        parent, context, RemoteCatalog(http)
    )

    override val name: String
        get() = context.getString(R.string.vfs_aminet_root_name)
    override val description: String
        get() = context.getString(R.string.vfs_aminet_root_description)

    override fun getExtension(id: String) = when {
        VfsExtensions.SEARCH_ENGINE == id && remote.searchSupported() -> SearchEngine()
        VfsExtensions.ICON == id -> R.drawable.ic_browser_vfs_aminet
        else -> super.getExtension(id)
    }

    override fun resolve(uri: Uri) = resolve(Path.parse(uri))

    private inner class SearchEngine : VfsExtensions.SearchEngine {
        override fun find(query: String, visitor: VfsExtensions.SearchEngine.Visitor) =
            remote.find(query, object : Catalog.DirVisitor {
                override fun acceptDir(name: String, description: String) {}
                override fun acceptFile(name: String, description: String, size: String) {
                    visitor.onFile(makeFile(rootPath.getChild(name), description, size))
                }
            })
    }
}
