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
import androidx.documentfile.provider.DocumentFile
import androidx.lifecycle.LiveData
import androidx.lifecycle.Transformations
import app.zxtune.Logger
import app.zxtune.device.PersistentStorage
import app.zxtune.playlist.Item
import java.io.IOException

class XspfStorage @VisibleForTesting constructor(
    private val resolver: ContentResolver,
    private val storage: LiveData<DocumentFile?>,
) {

    init {
        // Transformation is performed only on subscription
        storage.observeForever { dir ->
            LOG.d { "Using persistent storage at ${dir?.uri}" }
            rootCache = null
        }
    }

    private var rootCache: DocumentFile? = null
    private val root
        get() = rootCache ?: storage.value?.findFile(PLAYLISTS_DIR)?.takeIf { it.isDirectory }
            ?.also {
                rootCache = it
                LOG.d { "Reuse playlists dir ${it.uri}" }
            }
    private val rwRoot
        get() = root?.takeIf { it.isDirectory } ?: storage.value?.createDirectory(PLAYLISTS_DIR)
            ?.also {
                rootCache = it
                LOG.d { "Create playlists dir ${it.uri}" }
            }

    constructor(ctx: Context, storageState: LiveData<PersistentStorage.State>) : this(
        ctx.contentResolver, Transformations.map(storageState) { state -> state?.location })

    fun enumeratePlaylists() = ArrayList<String>().apply {
        root?.listFiles()?.forEach { doc ->
            val filename = doc.name.takeIf { doc.isFile } ?: return@forEach
            val extPos = filename.lastIndexOf(EXTENSION, ignoreCase = true)
            if (-1 != extPos) {
                add(filename.substring(0, extPos))
            }
        }
    }

    fun findPlaylistUri(name: String) =
        root?.findFile(makeFilename(name))?.takeIf { it.isFile }?.uri

    @Throws(IOException::class)
    fun createPlaylist(name: String, cursor: Cursor) {
        val doc = requireNotNull(rwRoot?.createFile("", name + EXTENSION)) {
            "cannot create file $name at ${rwRoot?.uri}"
        }
        LOG.d { "Created playlist $name at ${doc.uri}" }
        requireNotNull(resolver.openOutputStream(doc.uri)) { "cannot open output stream ${doc.uri}" }
            .use { stream ->
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
