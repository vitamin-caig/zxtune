package app.zxtune.playlist

import android.content.ContentResolver
import android.content.Context
import android.net.Uri
import androidx.annotation.VisibleForTesting
import app.zxtune.analytics.Analytics
import app.zxtune.playlist.IO.asStatistics
import app.zxtune.playlist.IO.toContentValues
import app.zxtune.playlist.IO.toPlaylist
import app.zxtune.playlist.IO.toTrack
import app.zxtune.ui.utils.observeChanges
import app.zxtune.ui.utils.query
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.FlowPreview
import kotlinx.coroutines.flow.debounce
import kotlinx.coroutines.flow.transform
import kotlinx.coroutines.withContext

class PlaylistContent(size: Int) : ArrayList<Track>(size)

class ProviderClient @VisibleForTesting constructor(
    private val resolver: ContentResolver,
    val id: Playlist.Id,
    private val dispatcher: CoroutineDispatcher,
) {
    private val playlistUri = Query.localPlaylistUri(id)

    fun add(track: Track.Metadata) = resolver.insert(playlistUri, track.toContentValues())

    fun notifyChanges() = resolver.notifyChange(playlistUri, null)

    fun observeContent() = observeChanges().transform {
        queryContent()?.let {
            emit(it)
        }
    }

    @OptIn(FlowPreview::class)
    fun observeChanges() = resolver.observeChanges(playlistUri).debounce(1000)

    suspend fun queryContent() = resolver.query(playlistUri) { cursor ->
        PlaylistContent(cursor.count).apply {
            while (cursor.moveToNext()) {
                add(cursor.toTrack())
            }
        }
    }

    suspend fun delete(tracks: Track.IdSet) {
        deleteItems(tracks)
        Analytics.sendPlaylistEvent(Analytics.PlaylistAction.DELETE, tracks.size)
    }

    suspend fun clear() {
        deleteItems(null)
        Analytics.sendPlaylistEvent(Analytics.PlaylistAction.DELETE, 0)
    }

    private suspend fun deleteItems(tracks: Track.IdSet?) = withContext(dispatcher) {
        Provider.delete(resolver, Playlist.OperationScope(id, tracks))
        notifyChanges()
    }

    suspend fun reorder(track: Track.Id, delta: Int) = withContext(dispatcher) {
        Provider.reorder(resolver, Track.FullIdentifier(id, track), delta)
        notifyChanges()
        Analytics.sendEvent("ui/playlist/reorder", "delta" to delta)
    }

    suspend fun sort(spec: Playlist.Sorting) = withContext(dispatcher) {
        Provider.sort(resolver, id, spec)
        notifyChanges()
        Analytics.sendEvent(
            "ui/playlist/sort",
            "by" to spec.by.name.lowercase(),
            "order" to spec.order.name.lowercase()
        )
    }

    suspend fun statistics(tracks: Track.IdSet?) = withContext(dispatcher) {
        Analytics.sendPlaylistEvent(Analytics.PlaylistAction.STATISTICS, tracks?.size ?: 0)
        Provider.statistics(resolver, Playlist.OperationScope(id, tracks))?.asStatistics()
    }

    @Throws(Exception::class)
    suspend fun save(name: String, tracks: Track.IdSet?) = withContext(dispatcher) {
        Provider.save(resolver, name, Playlist.OperationScope(id, tracks))
        Analytics.sendPlaylistEvent(Analytics.PlaylistAction.SAVE, tracks?.size ?: 0)
    }

    companion object {
        @JvmStatic
        fun create(ctx: Context) = ProviderClient(ctx.contentResolver, Playlist.DEFAULT_ID, Dispatchers.IO)

        fun createUri(id: Track.FullIdentifier): Uri = Query.itemUri(id)

        fun findId(uri: Uri) = Query.findTrackId(uri)
    }
}

class AggregatingProviderClient(private val resolver: ContentResolver, private val dispatcher: CoroutineDispatcher) {
    @OptIn(FlowPreview::class)
    fun observePlaylists() = resolver.observeChanges(Query.savedPlaylistsUri()).debounce(1000).transform {
        queryPlaylists()?.let {
            emit(it)
        }
    }

    @VisibleForTesting
    suspend fun queryPlaylists() = resolver.query(Query.savedPlaylistsUri()) { cursor ->
        ArrayList<Playlist>(cursor.count).apply {
            while (cursor.moveToNext()) {
                add(cursor.toPlaylist())
            }
        }
    }

    fun getPlaylist(playlist: Playlist.Id) = ProviderClient(resolver, playlist, dispatcher)

    // id => path
    suspend fun getSavedPlaylists(id: String? = null) = withContext(dispatcher) {
        resolver.query(Query.savedPlaylistsUri(), selection = id) { cursor ->
            HashMap<String, String>().apply {
                while (cursor.moveToNext()) {
                    put(cursor.getString(0), cursor.getString(1))
                }
            }
        }
    }

    companion object {
        fun create(ctx: Context) = AggregatingProviderClient(ctx.contentResolver, Dispatchers.IO)
    }
}
