package app.zxtune.playlist

import android.content.ContentResolver
import android.content.Context
import android.net.Uri
import androidx.annotation.VisibleForTesting
import app.zxtune.analytics.Analytics
import app.zxtune.playlist.IO.asStatistics
import app.zxtune.playlist.IO.toContentValues
import app.zxtune.playlist.IO.toTrack
import app.zxtune.ui.playlist.Entry
import app.zxtune.ui.utils.observeChanges
import app.zxtune.ui.utils.query
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.FlowPreview
import kotlinx.coroutines.flow.debounce
import kotlinx.coroutines.flow.transform
import kotlinx.coroutines.withContext

class PlaylistContent(size: Int) : ArrayList<Entry>(size)

class ProviderClient @VisibleForTesting constructor(
    private val resolver: ContentResolver,
    private val dispatcher: CoroutineDispatcher,
) {
    fun add(track: Track.Metadata) = resolver.insert(PlaylistQuery.ALL, track.toContentValues())

    fun notifyChanges() = resolver.notifyChange(PlaylistQuery.ALL, null)

    fun observeContent() = observeChanges().transform {
        queryContent()?.let {
            emit(it)
        }
    }

    @OptIn(FlowPreview::class)
    fun observeChanges() = resolver.observeChanges(PlaylistQuery.ALL).debounce(1000)

    suspend fun queryContent() = resolver.query(PlaylistQuery.ALL) { cursor ->
        PlaylistContent(cursor.count).apply {
            while (cursor.moveToNext()) {
                add(cursor.toTrack().run {
                    Entry(
                        id.value,
                        meta.location,
                        meta.title,
                        meta.author,
                        meta.duration,
                    )
                })
            }
        }
    }

    suspend fun delete(tracks: Track.IdSet) {
        deleteItems(tracks)
        Analytics.sendPlaylistEvent(Analytics.PlaylistAction.DELETE, tracks.size)
    }

    suspend fun deleteAll() {
        deleteItems(null)
        Analytics.sendPlaylistEvent(Analytics.PlaylistAction.DELETE, 0)
    }

    private suspend fun deleteItems(tracks: Track.IdSet?) = withContext(dispatcher) {
        Provider.delete(resolver, tracks)
        notifyChanges()
    }

    suspend fun reorder(track: Track.Id, delta: Int) = withContext(dispatcher) {
        Provider.reorder(resolver, track, delta)
        notifyChanges()
        Analytics.sendEvent("ui/playlist/reorder", "delta" to delta)
    }

    suspend fun sort(spec: Playlist.Sorting) = withContext(dispatcher) {
        Provider.sort(resolver, spec)
        notifyChanges()
        Analytics.sendEvent(
            "ui/playlist/sort",
            "by" to spec.by.name.lowercase(),
            "order" to spec.order.name.lowercase()
        )
    }

    suspend fun statistics(ids: LongArray?) = withContext(dispatcher) {
        Analytics.sendPlaylistEvent(Analytics.PlaylistAction.STATISTICS, ids?.size ?: 0)
        Provider.statistics(resolver, ids?.let { Track.IdSet(it) })?.asStatistics()
    }

    // id => path
    suspend fun getSavedPlaylists(id: String? = null) = withContext(dispatcher) {
        resolver.query(
            PlaylistQuery.SAVED, selection = id
        ) { cursor ->
            HashMap<String, String>().apply {
                while (cursor.moveToNext()) {
                    put(cursor.getString(0), cursor.getString(1))
                }
            }
        }
    }

    @Throws(Exception::class)
    suspend fun savePlaylist(id: String, ids: LongArray?) = withContext(dispatcher) {
        Provider.save(resolver, id, ids?.let { Track.IdSet(it) })
        Analytics.sendPlaylistEvent(Analytics.PlaylistAction.SAVE, ids?.size ?: 0)
    }

    companion object {
        @JvmStatic
        fun create(ctx: Context) = ProviderClient(ctx.contentResolver, Dispatchers.IO)

        fun createUri(id: Long): Uri = PlaylistQuery.uriFor(id)

        @JvmStatic
        fun findId(uri: Uri) =
            if (PlaylistQuery.isPlaylistUri(uri)) PlaylistQuery.idOf(uri) else null
    }
}
