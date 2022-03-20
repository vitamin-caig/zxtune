/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playlist.xspf

import android.content.Context
import android.database.Cursor
import android.os.Environment
import androidx.annotation.VisibleForTesting
import app.zxtune.Logger
import app.zxtune.R
import app.zxtune.playlist.Item
import java.io.File
import java.io.FileOutputStream
import java.io.IOException

class XspfStorage @VisibleForTesting constructor(root: File) {

    private val root: File = root
        get() = field.apply {
            if (mkdirs()) {
                LOG.d { "Created playlists storage dir $this" }
            }
        }

    constructor(context: Context) : this(
        File(
            Environment.getExternalStorageDirectory(),
            context.getString(R.string.playlists_storage_path)
        )
    )

    fun enumeratePlaylists() = ArrayList<String>().apply {
        root.list()?.forEach { filename ->
            val extPos = filename.lastIndexOf(EXTENSION, ignoreCase = true)
            if (-1 != extPos) {
                add(filename.substring(0, extPos))
            }
        }
    }

    fun findPlaylistPath(name: String) = getFileFor(name).takeIf { it.isFile }?.absolutePath

    @Throws(IOException::class)
    fun createPlaylist(name: String, cursor: Cursor) {
        FileOutputStream(getFileFor(name)).use { stream ->
            Builder(stream).apply {
                writePlaylistProperties(name, cursor.count)
                while (cursor.moveToNext()) {
                    writeTrack(Item(cursor))
                }
            }.finish()
        }
    }

    private fun getFileFor(name: String) = File(root, name + EXTENSION)

    companion object {
        private val LOG = Logger(XspfStorage::class.java.name)
        private const val EXTENSION = ".xspf"
    }
}
