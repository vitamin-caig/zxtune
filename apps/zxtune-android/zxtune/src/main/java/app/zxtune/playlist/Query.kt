package app.zxtune.playlist

import android.content.ContentResolver
import android.content.UriMatcher
import android.net.Uri
import app.zxtune.BuildConfig

/*
 * content://${authority}/saved - list saved playlists (name as a selection)
 * content://${authority}/local - list local playlists
 * content://${authority}/items[?playlist=$id] - all items by playlist
 *                       /items/#[?playlist=$id] - particular item for playlist
 */

object Query {
    enum class Type(val path: String, val arg: String) {
        SAVED("saved", ""),  // list saved playlists
        LOCAL("local", ""), // list local playlists
        ITEMS("items", ""), // list items
        ITEM("items", "#"),  // address item
        ;

        val mime
            get() = (if (arg.isEmpty()) ContentResolver.CURSOR_DIR_BASE_TYPE else ContentResolver.CURSOR_ITEM_BASE_TYPE) + "/vnd.$AUTHORITY.$path"
    }

    private const val AUTHORITY = "${BuildConfig.APPLICATION_ID}.playlist"
    private const val PLAYLIST_PARAM = "playlist"

    private val uriTemplate = UriMatcher(UriMatcher.NO_MATCH).apply {
        Type.entries.forEach {
            addURI(AUTHORITY, if (it.arg.isEmpty()) it.path else "${it.path}/${it.arg}", it.ordinal)
        }
    }

    val ROOT_URI = Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT).authority(AUTHORITY).build()

    fun getUriType(uri: Uri) = Type.entries.getOrNull(uriTemplate.match(uri))

    fun savedPlaylistsUri() = makeUri(Type.SAVED).build()
    fun localPlaylistsUri() = makeUri(Type.LOCAL).build()
    fun localPlaylistUri(playlist: Playlist.Id) =
        makeUri(Type.ITEMS).add(playlist).build()

    fun itemUri(id: Track.FullIdentifier) =
        makeUri(Type.ITEM).appendPath(id.track.value.toString()).add(id.playlist).build()

    fun getLocalPlaylistId(uri: Uri) = getUriType(uri).let {
        require(it == Type.ITEM || it == Type.ITEMS) { "Invalid local playlist uri $uri" }
        uri.getQueryParameter(PLAYLIST_PARAM)?.toPlaylistId() ?: Playlist.DEFAULT_ID
    }

    fun findTrackId(uri: Uri) =
        uri.takeIf { Type.ITEM == getUriType(uri) }?.lastPathSegment?.toTrackId()?.let {
            Track.FullIdentifier(
                uri.getQueryParameter(PLAYLIST_PARAM)?.toPlaylistId() ?: Playlist.DEFAULT_ID, it
            )
        }

    private fun String.toPlaylistId() = Playlist.Id(toLong())
    private fun String.toTrackId() = Track.Id(toLong())
    private fun Uri.Builder.add(id: Playlist.Id) = apply {
        if (id != Playlist.DEFAULT_ID) {
            appendQueryParameter(PLAYLIST_PARAM, id.value.toString())
        }
    }

    private fun makeUri(type: Type) = ROOT_URI.buildUpon().encodedPath(type.path)
}
