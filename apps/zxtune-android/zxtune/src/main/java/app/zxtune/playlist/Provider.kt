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
import app.zxtune.playlist.IO.asSorting
import app.zxtune.playlist.IO.getDelta
import app.zxtune.playlist.IO.getPlaylistId
import app.zxtune.playlist.IO.getTrackId
import app.zxtune.playlist.IO.getPlaylistOperationScope
import app.zxtune.playlist.IO.getTrackFullIdentifier
import app.zxtune.playlist.IO.putDelta
import app.zxtune.playlist.IO.putPlaylistId
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
    ): Cursor = when (Query.getUriType(uri)) {
        Query.Type.SAVED -> querySavedPlaylists(selection)
        Query.Type.LOCAL -> db.queryPlaylists()
        Query.Type.ITEMS -> db.getPlaylist(Query.getLocalPlaylistId(uri)).queryTracks().apply {
            setNotificationUri(resolver, uri)
        }

        else -> throw IllegalArgumentException("Invalid uri: $uri")
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
        requireNotNull(values)
        val playlist = Query.getLocalPlaylistId(uri)
        val track = db.getPlaylist(playlist).addTrack(values.toTrackMetadata())
        //do not notify about change
        return Query.itemUri(Track.FullIdentifier(playlist, track))
    }

    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<String>?) =
        TODO("Not implemented")

    override fun update(
        uri: Uri, values: ContentValues?, selection: String?, selectionArgs: Array<String>?
    ) = TODO("Not implemented")

    override fun call(method: String, arg: String?, extras: Bundle?): Bundle? {
        if (extras == null) {
            return null
        }
        when (method) {
            METHOD_STATISTICS -> return statistics(extras.getPlaylistOperationScope())
            METHOD_DELETE -> delete(extras.getPlaylistOperationScope())
            METHOD_REORDER -> ifNotNulls(
                extras.getTrackFullIdentifier(),
                extras.getDelta(),
                this::reorder
            )

            METHOD_SORT -> ifNotNulls(extras.getPlaylistId(), extras.asSorting(), this::sort)

            METHOD_SAVE -> ifNotNulls(arg, extras.getPlaylistOperationScope(), this::save)
        }
        return null
    }

    private fun sort(playlist: Playlist.Id, spec: Playlist.Sorting) =
        db.getPlaylist(playlist).sort(spec)

    private fun reorder(track : Track.FullIdentifier, delta: Int) =
        db.getPlaylist(track.playlist).reorder(track.track, delta)

    private fun save(name: String, scope: Playlist.OperationScope) = (scope.tracks?.let {
        db.queryTracks(it)
    } ?: db.getPlaylist(scope.playlist).queryTracks()).use { cursor ->
        runCatching {
            runBlocking {
                storage.createPlaylist(name, cursor)
            }
            null
        }.recover { err ->
            Log.w(TAG, err, "Failed to save")
            bundleOf("error" to err)
        }.getOrNull()
    }

    private fun statistics(scope: Playlist.OperationScope) = (scope.tracks?.let {
        db.queryStatistics(it)
    } ?: db.getPlaylist(scope.playlist).queryStatistics()).toBundle()

    private fun delete(scope: Playlist.OperationScope) =
        with(db.getPlaylist(scope.playlist)) {
            // TODO: delete or clear?
            scope.tracks?.let {
                deleteTracks(it)
            } ?: delete()
        }

    override fun getType(uri: Uri) = Query.getUriType(uri)?.mime

    companion object {
        private val TAG: String = Provider::class.java.name

        private const val METHOD_SORT = "sort"
        private const val METHOD_REORDER = "reorder"
        private const val METHOD_SAVE = "save"
        private const val METHOD_STATISTICS = "statistics"
        private const val METHOD_DELETE = "delete"

        fun sort(resolver: ContentResolver, playlist: Playlist.Id, spec: Playlist.Sorting) =
            resolver.call(Query.ROOT_URI, METHOD_SORT, null, spec.toBundle().apply {
                putPlaylistId(playlist)
            })

        fun reorder(
            resolver: ContentResolver, track: Track.FullIdentifier, delta: Int
        ) = resolver.call(
            Query.ROOT_URI, METHOD_REORDER, null, track.toBundle().apply {
                putDelta(delta)
            })

        fun save(resolver: ContentResolver, name: String, scope: Playlist.OperationScope) =
            resolver.call(Query.ROOT_URI, METHOD_SAVE, name, scope.toBundle())?.run {
                throw getSerializable("error") as Throwable
            }

        fun statistics(resolver: ContentResolver, scope: Playlist.OperationScope) =
            resolver.call(Query.ROOT_URI, METHOD_STATISTICS, null, scope.toBundle())

        fun delete(resolver: ContentResolver, scope: Playlist.OperationScope) =
            resolver.call(Query.ROOT_URI, METHOD_DELETE, null, scope.toBundle())
    }
}