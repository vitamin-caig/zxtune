package app.zxtune.analytics

import android.content.Context
import android.net.Uri
import androidx.annotation.IntDef
import androidx.collection.LongSparseArray
import app.zxtune.core.Player
import app.zxtune.playback.PlayableItem

object Analytics {

    private var sink: Sink? = null

    @JvmStatic
    fun initialize(ctx: Context) {
        sink = InternalSink(ctx)
    }

    @JvmStatic
    fun logException(e: Throwable) = sink?.logException(e) ?: Unit

    private fun sendTrace(id: String, points: LongSparseArray<String>) = sink?.sendTrace(
        id, points
    ) ?: Unit

    @JvmStatic
    fun sendPlayEvent(item: PlayableItem, player: Player) =
        sink?.sendPlayEvent(item, player) ?: Unit

    const val BROWSER_ACTION_BROWSE: Int = 0
    const val BROWSER_ACTION_BROWSE_PARENT: Int = 1
    const val BROWSER_ACTION_SEARCH: Int = 2

    fun sendBrowserEvent(path: Uri, @BrowserAction action: Int) =
        sink?.sendBrowserEvent(path, action) ?: Unit

    const val SOCIAL_ACTION_RINGTONE: Int = 0
    const val SOCIAL_ACTION_SHARE: Int = 1
    const val SOCIAL_ACTION_SEND: Int = 2

    fun sendSocialEvent(path: Uri, app: String, @SocialAction action: Int) =
        sink?.sendSocialEvent(path, app, action) ?: Unit

    const val UI_ACTION_OPEN: Int = 0
    const val UI_ACTION_CLOSE: Int = 1
    const val UI_ACTION_PREFERENCES: Int = 2
    const val UI_ACTION_RATE: Int = 3
    const val UI_ACTION_ABOUT: Int = 4
    const val UI_ACTION_QUIT: Int = 5

    fun sendUiEvent(@UiAction action: Int) = sink?.sendUiEvent(action) ?: Unit

    const val PLAYLIST_ACTION_ADD: Int = 0
    const val PLAYLIST_ACTION_DELETE: Int = 1
    const val PLAYLIST_ACTION_MOVE: Int = 2
    const val PLAYLIST_ACTION_SORT: Int = 3
    const val PLAYLIST_ACTION_SAVE: Int = 4
    const val PLAYLIST_ACTION_STATISTICS: Int = 5

    @JvmStatic
    fun sendPlaylistEvent(@PlaylistAction action: Int, param: Int) =
        sink?.sendPlaylistEvent(action, param) ?: Unit

    const val VFS_ACTION_REMOTE_FETCH: Int = 0
    const val VFS_ACTION_REMOTE_FALLBACK: Int = 1
    const val VFS_ACTION_CACHED_FETCH: Int = 2
    const val VFS_ACTION_CACHED_FALLBACK: Int = 3

    private fun sendVfsEvent(id: String, scope: String, @VfsAction action: Int, duration: Long) =
        sink?.sendVfsEvent(id, scope, action, duration) ?: Unit

    @JvmStatic
    fun sendNoTracksFoundEvent(uri: Uri) = sink?.sendNoTracksFoundEvent(uri) ?: Unit

    @JvmStatic
    fun sendDbMetrics(name: String, size: Long, tablesRows: HashMap<String, Long>) =
        sink?.sendDbMetrics(name, size, tablesRows) ?: Unit

    @JvmStatic
    fun sendEvent(id: String, vararg arguments: Pair<String, Any?>) = sink?.sendEvent(
        id, *arguments
    ) ?: Unit

    // TODO: replace by annotation
    abstract class BaseTrace protected constructor(private val id: String) {
        private val points = LongSparseArray<String>()
        private var method = ""

        open fun beginMethod(name: String) {
            points.clear()
            method = name
            checkpoint("in")
        }

        fun checkpoint(point: String) = points.append(metric, point)

        open fun endMethod() {
            checkpoint("out")
            sendTrace("$id.$method", points)
            method = ""
        }

        protected abstract val metric: Long
    }

    class Trace private constructor(id: String) : BaseTrace(id) {
        private val start = System.nanoTime()

        override val metric: Long
            get() = ((System.nanoTime() - start) / 1000)

        companion object {
            @JvmStatic
            fun create(id: String) = Trace(id)
        }
    }

    class StageDurationTrace private constructor(id: String) : BaseTrace(id) {
        private val createTime = System.nanoTime()
        private var methodStart: Long = 0
        private var stageStart: Long = 0

        // 'in' measures from create to beginMethod
        override fun beginMethod(name: String) {
            methodStart = createTime
            stageStart = methodStart
            super.beginMethod(name)
        }

        // 'out' measures from in to out
        override fun endMethod() {
            stageStart = methodStart
            super.endMethod()
        }

        override val metric: Long
            get() {
                val prev = stageStart
                stageStart = System.nanoTime()
                return (stageStart - prev) / 1000
            }

        companion object {
            fun create(id: String) = StageDurationTrace(id)
        }
    }

    @Retention(AnnotationRetention.SOURCE)
    @IntDef(BROWSER_ACTION_BROWSE, BROWSER_ACTION_BROWSE_PARENT, BROWSER_ACTION_SEARCH)
    internal annotation class BrowserAction

    @Retention(AnnotationRetention.SOURCE)
    @IntDef(SOCIAL_ACTION_RINGTONE, SOCIAL_ACTION_SHARE, SOCIAL_ACTION_SEND)
    internal annotation class SocialAction

    @Retention(AnnotationRetention.SOURCE)
    @IntDef(
        UI_ACTION_OPEN,
        UI_ACTION_CLOSE,
        UI_ACTION_PREFERENCES,
        UI_ACTION_RATE,
        UI_ACTION_ABOUT,
        UI_ACTION_QUIT
    )
    internal annotation class UiAction

    @Retention(AnnotationRetention.SOURCE)
    @IntDef(
        PLAYLIST_ACTION_ADD,
        PLAYLIST_ACTION_DELETE,
        PLAYLIST_ACTION_MOVE,
        PLAYLIST_ACTION_SORT,
        PLAYLIST_ACTION_SAVE,
        PLAYLIST_ACTION_STATISTICS
    )
    internal annotation class PlaylistAction

    @Retention(AnnotationRetention.SOURCE)
    @IntDef(
        VFS_ACTION_REMOTE_FETCH,
        VFS_ACTION_REMOTE_FALLBACK,
        VFS_ACTION_CACHED_FETCH,
        VFS_ACTION_CACHED_FALLBACK
    )
    internal annotation class VfsAction

    class VfsTrace private constructor(private val id: String, private val scope: String) {
        private val start = System.currentTimeMillis()

        fun send(@VfsAction action: Int) {
            val duration = System.currentTimeMillis() - start
            sendVfsEvent(id, scope, action, duration)
        }

        companion object {
            @JvmStatic
            fun create(id: String, scope: String): VfsTrace {
                return VfsTrace(id, scope)
            }
        }
    }
}
