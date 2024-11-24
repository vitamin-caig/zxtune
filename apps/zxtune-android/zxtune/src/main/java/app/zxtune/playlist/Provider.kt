/**
 * @file
 * @brief Playlist content provider component
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playlist

import android.content.ContentProvider
import android.content.ContentResolver
import android.content.ContentValues
import android.database.Cursor
import android.database.MatrixCursor
import android.net.Uri
import android.os.Bundle
import android.util.SparseIntArray
import androidx.core.os.bundleOf
import app.zxtune.Log
import app.zxtune.MainApplication
import app.zxtune.playlist.Database.Tables.Playlist
import app.zxtune.playlist.xspf.XspfStorage
import kotlinx.coroutines.runBlocking
import kotlin.math.abs

class Provider : ContentProvider() {
    private lateinit var db: Database
    private lateinit var storage: XspfStorage
    private lateinit var resolver: ContentResolver

    override fun onCreate() = context?.run {
        MainApplication.initialize(applicationContext)
        db = Database(this)
        storage = XspfStorage(this)
        resolver = contentResolver
        true
    } ?: false

    override fun query(
        uri: Uri,
        projection: Array<String>?,
        selection: String?,
        selectionArgs: Array<String>?,
        sortOrder: String?
    ): Cursor = when (uri) {
        PlaylistQuery.STATISTICS -> db.queryStatistics(selection)
        PlaylistQuery.SAVED -> querySavedPlaylists(selection)
        else -> {
            val select =
                PlaylistQuery.idOf(uri)?.let { PlaylistQuery.selectionFor(it) } ?: selection
            db.queryPlaylistItems(projection, select, selectionArgs, sortOrder).apply {
                setNotificationUri(resolver, PlaylistQuery.ALL)
            }
        }
    }

    private fun querySavedPlaylists(selection: String?) =
        MatrixCursor(arrayOf("name", "path")).apply {
            runBlocking {
                val ids = selection?.let { arrayListOf(it) } ?: storage.enumeratePlaylists()
                for (id in ids) {
                    storage.findPlaylistUri(id)?.let { uri ->
                        addRow(arrayOf(id, uri.toString()))
                    }
                }
            }
        }

    override fun insert(uri: Uri, values: ContentValues?): Uri? {
        require(null == PlaylistQuery.idOf(uri)) { "Wrong URI: $uri" }
        val result = db.insertPlaylistItem(values)
        //do not notify about change
        return PlaylistQuery.uriFor(result)
    }

    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<String>?) =
        PlaylistQuery.idOf(uri)?.let { id ->
            db.deletePlaylistItems(PlaylistQuery.selectionFor(id), null)
        } ?: db.deletePlaylistItems(selection, selectionArgs)

    override fun update(
        uri: Uri, values: ContentValues?, selection: String?, selectionArgs: Array<String>?
    ) = PlaylistQuery.idOf(uri)?.let { id ->
        db.updatePlaylistItems(values, PlaylistQuery.selectionFor(id), null)
    } ?: db.updatePlaylistItems(values, selection, selectionArgs)


    override fun call(method: String, arg: String?, extras: Bundle?) = when {
        arg == null -> null
        METHOD_SAVE == method -> save(arg, extras!!.getLongArray("ids"))
        METHOD_SORT == method -> {
            sort(arg.substringBefore(' '), arg.substringAfter(' '))
            null
        }

        METHOD_MOVE == method -> {
            move(arg.substringBefore(' ').toLong(), arg.substringAfter(' ').toInt())
            null
        }

        else -> null
    }

    private fun sort(fieldName: String, order: String) = db.sortPlaylistItems(
        Playlist.Fields.valueOf(fieldName), order
    )

    /*
    * @param id item to move
    * @param delta position change
    *//*
   * pos idx     pos idx
   *       <-+
   * p0  i0  |   p0  i0 -+
   * p1  i1  |   p1  i1  |
   * p2  i2  |   p2  i2  |
   * p3  i3  |   p3  i3  |
   * p4  i4  |   p4  i4  |
   * p5  i5 -+   p5  i5  |
   *                   <-+
   *
   * move(i5,-5) move(i0,5)
   *
   * select:
   * i5 i4 i3 i2 i1 i0
   * p5 p4 p3 p2 p1 p0
   *
   *             i0 i1 i2 i3 i4 i5
   *             p0 p1 p2 p3 p4 p5
   *
   * p0  i5      p0  i1
   * p1  i0      p1  i2
   * p2  i1      p2  i3
   * p3  i2      p3  i4
   * p4  i3      p4  i5
   * p5  i4      p5  i0
   *
   */
    private fun move(id: Long, delta: Int) = getNewPositions(id, delta).let {
        db.updatePlaylistItemsOrder(it)
    }

    // TODO:
    //  - SparseLongArray and revert mapping
    //  - batch processing in updatePlaylistItemsOrder
    private fun getNewPositions(id: Long, delta: Int) = SparseIntArray().apply {
        val proj = arrayOf(Playlist.Fields._id.name, Playlist.Fields.pos.name)
        val sel = PlaylistQuery.positionSelection(if (delta > 0) ">=" else "<=", id)
        val count = (abs(delta.toDouble()) + 1).toInt()
        val ord = PlaylistQuery.limitedOrder(if (delta > 0) delta + 1 else delta - 1)
        val ids = IntArray(count)
        val pos = IntArray(count)
        db.queryPlaylistItems(proj, sel, null, ord).use { cursor ->
            var i = 0
            while (cursor.moveToNext()) {
                ids[(i + count - 1) % count] = cursor.getInt(0)
                pos[i] = cursor.getInt(1)
                ++i
            }
        }
        for (i in 0 until count) {
            append(ids[i], pos[i])
        }
    }

    private fun save(id: String, ids: LongArray?) = db.queryPlaylistItems(
        null, PlaylistQuery.selectionFor(ids), null, null
    ).use { cursor ->
        runCatching {
            runBlocking {
                storage.createPlaylist(id, cursor)
            }
            null
        }.recover { err ->
            Log.w(TAG, err, "Failed to save")
            bundleOf("error" to err)
        }.getOrNull()
    }

    override fun getType(uri: Uri) = runCatching {
        PlaylistQuery.mimeTypeOf(uri)
    }.getOrNull()

    companion object {
        private val TAG: String = Provider::class.java.name

        private const val METHOD_SORT = "sort"
        private const val METHOD_MOVE = "move"
        private const val METHOD_SAVE = "save"

        fun sort(resolver: ContentResolver, by: String, order: String) = resolver.call(
            PlaylistQuery.ALL, METHOD_SORT, "$by $order", null
        )

        fun move(resolver: ContentResolver, id: Long, delta: Int) = resolver.call(
            PlaylistQuery.ALL, METHOD_MOVE, "$id $delta", null
        )

        fun save(resolver: ContentResolver, id: String?, ids: LongArray?) =
            resolver.call(PlaylistQuery.ALL, METHOD_SAVE, id, bundleOf("ids" to ids))?.run {
                throw getSerializable("error") as Throwable
            }
    }
}