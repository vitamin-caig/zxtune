/**
 * @file
 * @brief Implementation of VfsRoot over http://hvsc.c64.org collection
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs

import android.content.Context
import android.net.Uri
import app.zxtune.R
import app.zxtune.fs.aygor.Path
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.fs.httpdir.Catalog
import app.zxtune.fs.httpdir.HttpRootBase

internal class VfsRootAygor(
    parent: VfsObject,
    private val context: Context,
    http: MultisourceHttpProvider
) : HttpRootBase(parent, Catalog.create(context, http, "aygor"), Path.create()), VfsRoot {

    override val name: String
        get() = context.getString(R.string.vfs_aygor_root_name)
    override val description: String
        get() = context.getString(R.string.vfs_aygor_root_description)

    override fun getExtension(id: String) = when (id) {
        VfsExtensions.ICON -> R.drawable.ic_browser_vfs_aygor
        else -> super.getExtension(id)
    }

    override fun resolve(uri: Uri) = resolve(Path.parse(uri))
}
