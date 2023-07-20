package app.zxtune.analytics

import android.net.Uri
import androidx.collection.LongSparseArray
import app.zxtune.core.Player
import app.zxtune.playback.PlayableItem

internal interface Sink {
    fun logException(e: Throwable)

    // elapsed => tag
    fun sendTrace(id: String, points: LongSparseArray<String>)
    fun sendPlayEvent(item: PlayableItem, player: Player)
    fun sendBrowserEvent(path: Uri, @Analytics.BrowserAction action: Int)
    fun sendSocialEvent(path: Uri, app: String, @Analytics.SocialAction action: Int)
    fun sendUiEvent(@Analytics.UiAction action: Int)

    //! @param scopeSize - uris count for add, selection size else
    //TODO: cleanup
    fun sendPlaylistEvent(@Analytics.PlaylistAction action: Int, param: Int)
    fun sendVfsEvent(id: String, scope: String, @Analytics.VfsAction action: Int, duration: Long)
    fun sendNoTracksFoundEvent(uri: Uri)
    fun sendDbMetrics(name: String, size: Long, tablesRows: HashMap<String, Long>)

    fun sendEvent(id: String, vararg arguments: Pair<String, *>)
}
