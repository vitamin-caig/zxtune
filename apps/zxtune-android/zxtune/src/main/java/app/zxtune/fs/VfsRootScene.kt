/**
 * @file
 * @brief Implementation of VfsRoot over http://asma.atari.org collection
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs

import android.content.Context
import android.net.Uri
import app.zxtune.R
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.fs.httpdir.Catalog
import app.zxtune.fs.httpdir.HttpRootBase
import app.zxtune.fs.scene.Path

internal class VfsRootScene(
    parent: VfsObject,
    private val context: Context,
    http: MultisourceHttpProvider
) : HttpRootBase(parent, Catalog.create(context, http, "scene"), Path.create()), VfsRoot {

    override val name: String
        get() = context.getString(R.string.vfs_scene_root_name)
    override val description: String
        get() = context.getString(R.string.vfs_scene_root_description)

    override fun getExtension(id: String) = when (id) {
        VfsExtensions.ICON -> R.drawable.ic_browser_vfs_scene
        else -> super.getExtension(id)
    }

    override fun resolve(uri: Uri) = resolve(Path.parse(uri))
}
