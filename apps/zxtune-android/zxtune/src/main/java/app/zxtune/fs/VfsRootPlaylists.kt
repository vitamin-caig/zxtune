/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs

import android.content.Context
import android.net.Uri
import app.zxtune.R
import app.zxtune.playlist.ProviderClient.Companion.create
import kotlinx.coroutines.runBlocking
import java.io.FileNotFoundException

internal class VfsRootPlaylists(private val context: Context) : StubObject(), VfsRoot {
    private val client by lazy { create(context) }

    override val uri: Uri
        get() = rootUriBuilder().build()

    override val name
        get() = context.getString(R.string.vfs_playlists_root_name)

    override val description
        get() = context.getString(R.string.vfs_playlists_root_description)

    override val parent
        get() = null

    override fun enumerate(visitor: VfsDir.Visitor) = runBlocking {
        client.getSavedPlaylists()?.let { entries ->
            for ((name, path) in entries) {
                visitor.onFile(PlaylistFile(name, path))
            }
        } ?: Unit
    }

    override fun resolve(uri: Uri): VfsObject? {
        if (SCHEME == uri.scheme) {
            val path = uri.pathSegments
            if (path.isEmpty()) {
                return this
            } else if (path.size == 1) {
                val name = path[0]
                return PlaylistFile(name)
            }
        }
        return null
    }

    private inner class PlaylistFile(override val name: String, private var path: String? = null) :
        StubObject(), VfsFile {
        override val uri: Uri
            get() = rootUriBuilder().appendPath(name).build()

        override val parent
            get() = this@VfsRootPlaylists

        override val size: String
            get() = description

        override fun getExtension(id: String) = when (id) {
            VfsExtensions.INPUT_STREAM -> openStream()
            else -> super.getExtension(id)
        }

        private fun openStream() = try {
            if (path == null) {
                path = runBlocking { client.getSavedPlaylists(name)?.get(name) }
            }
            path?.let { context.contentResolver.openInputStream(Uri.parse(it)) }
        } catch (e: FileNotFoundException) {
            null
        }
    }

    companion object {
        const val SCHEME: String = "playlists"

        private fun rootUriBuilder() = Uri.Builder().scheme(SCHEME)
    }
}
