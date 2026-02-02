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
import androidx.core.os.bundleOf
import app.zxtune.Log
import app.zxtune.MainApplication
import app.zxtune.playlist.Database.Tables.Playlist
import app.zxtune.playlist.IO.getDelta
import app.zxtune.playlist.IO.getTrackId
import app.zxtune.playlist.IO.getTrackIdSet
import app.zxtune.playlist.IO.putDelta
import app.zxtune.playlist.IO.putTrackId
import app.zxtune.playlist.IO.toBundle
import app.zxtune.playlist.IO.toTrackMetadata
import app.zxtune.playlist.xspf.XspfStorage
import app.zxtune.utils.ifNotNulls
import kotlinx.coroutines.runBlocking

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
        requireNotNull(values)
        val playlist = db.getPlaylist()
        val result = playlist.addTrack(values.toTrackMetadata())
        //do not notify about change
        return PlaylistQuery.uriFor(result.value)
    }

    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<String>?) =
        TODO("Not implemented")

    override fun update(
        uri: Uri, values: ContentValues?, selection: String?, selectionArgs: Array<String>?
    ) = TODO("Not implemented")

    override fun call(method: String, arg: String?, extras: Bundle?) = when {
        METHOD_STATISTICS == method -> statistics(extras?.getTrackIdSet())
        METHOD_DELETE == method -> {
            delete(extras?.getTrackIdSet())
            null
        }
        METHOD_REORDER == method -> {
            ifNotNulls(extras?.getTrackId(), extras?.getDelta(), this::reorder)
            null
        }

        arg == null -> null
        METHOD_SAVE == method -> save(arg, extras!!.getLongArray("ids"))
        METHOD_SORT == method -> {
            sort(arg.substringBefore(' '), arg.substringAfter(' '))
            null
        }

        else -> null
    }

    private fun sort(fieldName: String, order: String) = db.sortPlaylistItems(
        Playlist.Fields.valueOf(fieldName), order
    )

    private fun reorder(track: Track.Id, delta: Int) = db.getPlaylist().reorder(track, delta)

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

    private fun statistics(tracks: Track.IdSet?) = (tracks?.let {
        db.queryStatistics(it)
    } ?: db.getPlaylist().queryStatistics()).toBundle()

    private fun delete(tracks: Track.IdSet?) = with(db.getPlaylist()) {
        tracks?.let {
            deleteTracks(it)
        } ?: delete()
    }

    override fun getType(uri: Uri) = runCatching {
        PlaylistQuery.mimeTypeOf(uri)
    }.getOrNull()

    companion object {
        private val TAG: String = Provider::class.java.name

        private const val METHOD_SORT = "sort"
        private const val METHOD_REORDER = "reorder"
        private const val METHOD_SAVE = "save"
        private const val METHOD_STATISTICS = "statistics"
        private const val METHOD_DELETE = "delete"

        fun sort(resolver: ContentResolver, by: String, order: String) = resolver.call(
            PlaylistQuery.ALL, METHOD_SORT, "$by $order", null
        )

        fun reorder(resolver: ContentResolver, track: Track.Id, delta: Int) = resolver.call(
            PlaylistQuery.ALL, METHOD_REORDER, null, Bundle().apply {
                putTrackId(track)
                putDelta(delta)
            }
        )

        fun save(resolver: ContentResolver, id: String?, ids: LongArray?) =
            resolver.call(PlaylistQuery.ALL, METHOD_SAVE, id, bundleOf("ids" to ids))?.run {
                throw getSerializable("error") as Throwable
            }

        fun statistics(resolver: ContentResolver, tracks: Track.IdSet?) =
            resolver.call(PlaylistQuery.ALL, METHOD_STATISTICS, null, tracks?.toBundle())

        fun delete(resolver: ContentResolver, tracks: Track.IdSet?) =
            resolver.call(PlaylistQuery.ALL, METHOD_DELETE, null, tracks?.toBundle())
    }
}