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
    fun sendBrowserEvent(path: Uri, action: Analytics.BrowserAction)
    fun sendSocialEvent(path: Uri, app: String, action: Analytics.SocialAction)
    fun sendUiEvent(action: Analytics.UiAction)

    //! @param scopeSize - uris count for add, selection size else
    //TODO: cleanup
    fun sendPlaylistEvent(action: Analytics.PlaylistAction, param: Int)
    fun sendVfsEvent(id: String, scope: String, action: Analytics.VfsAction, duration: Long)
    fun sendNoTracksFoundEvent(uri: Uri)
    fun sendDbMetrics(name: String, size: Long, tablesRows: HashMap<String, Long>)

    fun sendEvent(id: String, vararg arguments: Pair<String, *>)
}
