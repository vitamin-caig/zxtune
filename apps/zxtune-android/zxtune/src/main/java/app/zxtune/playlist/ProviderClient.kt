package app.zxtune.playlist

import android.content.ContentResolver
import android.content.Context
import android.database.ContentObserver
import android.database.Cursor
import android.net.Uri
import androidx.annotation.VisibleForTesting
import androidx.tracing.trace
import app.zxtune.Logger
import app.zxtune.TimeStamp
import app.zxtune.analytics.Analytics
import app.zxtune.core.Identifier
import app.zxtune.ui.playlist.Entry
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.flow.callbackFlow

class ProviderClient @VisibleForTesting constructor(private val resolver: ContentResolver) {
    //should be name-compatible with Database
    enum class SortBy {
        title, author, duration
    }

    enum class SortOrder {
        asc, desc
    }

    fun addItem(item: Item) {
        resolver.insert(PlaylistQuery.ALL, item.toContentValues())
    }

    fun notifyChanges() = resolver.notifyChange(PlaylistQuery.ALL, null)

    fun observeChanges() = callbackFlow {
        val observer = object : ContentObserver(null) {
            override fun onChange(selfChange: Boolean) {
                trySend(Unit)
            }
        }
        LOG.d { "Subscribe for changes " }
        resolver.registerContentObserver(PlaylistQuery.ALL, true, observer)
        trySend(Unit)
        awaitClose {
            LOG.d { "Unsubscribe from changes" }
            resolver.unregisterContentObserver(observer)
        }
    }

    fun query(ids: LongArray?) = trace("Playlist.query") {
        resolver.query(
            PlaylistQuery.ALL, null, PlaylistQuery.selectionFor(ids), null, null
        )?.use { cursor ->
            ArrayList<Entry>(cursor.count).apply {
                while (cursor.moveToNext()) {
                    add(createItem(cursor))
                }
            }
        }
    }

    fun delete(ids: LongArray) {
        deleteItems(PlaylistQuery.selectionFor(ids))
        Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_DELETE, ids.size)
    }

    fun deleteAll() {
        deleteItems(null)
        Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_DELETE, 0)
    }

    private fun deleteItems(selection: String?) {
        resolver.delete(PlaylistQuery.ALL, selection, null)
        notifyChanges()
    }

    fun move(id: Long, delta: Int) {
        Provider.move(resolver, id, delta)
        notifyChanges()
        Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_MOVE, 1)
    }

    fun sort(by: SortBy, order: SortOrder) {
        Provider.sort(resolver, by.name, order.name)
        notifyChanges()
        Analytics.sendPlaylistEvent(
            Analytics.PLAYLIST_ACTION_SORT, 100 * by.ordinal + order.ordinal
        )
    }

    fun statistics(ids: LongArray?) = trace("Playlist.statistics") {
        resolver.query(
            PlaylistQuery.STATISTICS, null, PlaylistQuery.selectionFor(ids), null, null
        )?.use {
            if (it.moveToFirst()) {
                Statistics(it)
            } else {
                null
            }
        }
    }

    // id => path
    fun getSavedPlaylists(id: String?) = HashMap<String, String>().apply {
        resolver.query(PlaylistQuery.SAVED, null, id, null, null)?.use {
            while (it.moveToNext()) {
                put(it.getString(0), it.getString(1))
            }
        }
    }

    @Throws(Exception::class)
    fun savePlaylist(id: String, ids: LongArray?) {
        Provider.save(resolver, id, ids)
        Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_SAVE, ids?.size ?: 0)
    }

    companion object {
        private val LOG = Logger(ProviderClient::class.java.name)
        private fun createItem(cursor: Cursor) = cursor.run {
            Entry(
                getLong(Database.Tables.Playlist.Fields._id.ordinal),
                Identifier.parse(getString(Database.Tables.Playlist.Fields.location.ordinal)),
                getString(Database.Tables.Playlist.Fields.title.ordinal),
                getString(Database.Tables.Playlist.Fields.author.ordinal),
                TimeStamp.fromMilliseconds(getLong(Database.Tables.Playlist.Fields.duration.ordinal))
            )
        }

        @JvmStatic
        fun create(ctx: Context) = ProviderClient(ctx.contentResolver)

        fun createUri(id: Long): Uri = PlaylistQuery.uriFor(id)

        @JvmStatic
        fun findId(uri: Uri) =
            if (PlaylistQuery.isPlaylistUri(uri)) PlaylistQuery.idOf(uri) else null
    }
}
