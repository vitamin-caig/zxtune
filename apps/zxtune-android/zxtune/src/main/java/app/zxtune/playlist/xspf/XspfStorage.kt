/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playlist.xspf

import android.content.ContentResolver
import android.content.Context
import android.database.Cursor
import androidx.annotation.VisibleForTesting
import app.zxtune.Logger
import app.zxtune.device.PersistentStorage
import app.zxtune.playlist.Item
import java.io.IOException

class XspfStorage @VisibleForTesting constructor(
    private val resolver: ContentResolver,
    private val storage: PersistentStorage.Subdirectory,
) {
    constructor(ctx: Context) : this(
        ctx.contentResolver, PersistentStorage.instance.subdirectory(PLAYLISTS_DIR)
    )

    fun enumeratePlaylists() = ArrayList<String>().apply {
        storage.tryGet()?.listFiles()?.forEach { doc ->
            val filename = doc.name.takeIf { doc.isFile } ?: return@forEach
            val extPos = filename.lastIndexOf(EXTENSION, ignoreCase = true)
            if (-1 != extPos) {
                add(filename.substring(0, extPos))
            }
        }
    }

    fun findPlaylistUri(name: String) =
        storage.tryGet()?.findFile(makeFilename(name))?.takeIf { it.isFile }?.uri

    @Throws(IOException::class)
    fun createPlaylist(name: String, cursor: Cursor) {
        val targetName = name + EXTENSION
        val dir = storage.tryGet(createIfAbsent = true)
        // TODO: transactional
        val doc = checkNotNull(
            dir?.findFile(targetName) ?: dir?.createFile("", targetName)
        ) {
            "cannot create file $name"
        }
        LOG.d { "Playlist $name at ${doc.uri}" }
        checkNotNull(resolver.openOutputStream(doc.uri)) { "cannot open output stream ${doc.uri}" }.use { stream ->
            Builder(stream).apply {
                writePlaylistProperties(name, cursor.count)
                while (cursor.moveToNext()) {
                    writeTrack(Item(cursor))
                }
            }.finish()
        }
    }

    companion object {
        private val LOG = Logger(XspfStorage::class.java.name)
        private const val EXTENSION = ".xspf"

        private fun makeFilename(name: String) = name + EXTENSION

        // TODO: localize?
        private const val PLAYLISTS_DIR = "Playlists"
    }
}
