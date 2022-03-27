/**
 * @file
 * @brief Implementation of VfsRoot over http://hvsc.c64.org collection
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs

import android.content.Context
import android.net.Uri
import app.zxtune.R
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.fs.httpdir.Catalog
import app.zxtune.fs.httpdir.HttpRootBase
import app.zxtune.fs.hvsc.Path

internal class VfsRootHvsc(
    parent: VfsObject,
    private val context: Context,
    http: MultisourceHttpProvider
) : HttpRootBase(parent, Catalog.create(context, http, "hvsc"), Path.create()), VfsRoot {

    override val name: String
        get() = context.getString(R.string.vfs_hvsc_root_name)
    override val description: String
        get() = context.getString(R.string.vfs_hvsc_root_description)

    override fun enumerate(visitor: VfsDir.Visitor) = SUBDIRS.forEach { dir ->
        visitor.onDir(makeDir(rootPath.getChild(dir), ""))
    }

    override fun getExtension(id: String) = when (id) {
        VfsExtensions.ICON -> R.drawable.ic_browser_vfs_hvsc
        else -> super.getExtension(id)
    }

    override fun resolve(uri: Uri) = resolve(Path.parse(uri))

    companion object {
        private val SUBDIRS = arrayOf("DEMOS/", "GAMES/", "MUSICIANS/")
    }
}
