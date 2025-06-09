package app.zxtune.analytics

import android.content.Context
import android.net.Uri
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

    enum class BrowserAction(val key: String) {
        BROWSE("browse"), BROWSE_PARENT("up"), SEARCH("search"),
    }

    fun sendBrowserEvent(path: Uri, action: BrowserAction) =
        sink?.sendBrowserEvent(path, action) ?: Unit

    enum class SocialAction(val key: String) {
        RINGTONE("ringtone"), SHARE("share"), SEND("send"),
    }

    fun sendSocialEvent(path: Uri, app: String, action: SocialAction) =
        sink?.sendSocialEvent(path, app, action) ?: Unit

    enum class UiAction(val key: String) {
        OPEN("open"), CLOSE("close"), PREFERENCES("preferences"), RATE("rate"), ABOUT("about"), QUIT(
            "quit"
        ),
    }

    @JvmStatic
    fun sendUiEvent(action: UiAction) = sink?.sendUiEvent(action) ?: Unit

    enum class PlaylistAction(val key: String) {
        ADD("add"), DELETE("delete"), MOVE("move"), SORT("sort"), SAVE("save"), STATISTICS("statistics"),
    }

    @JvmStatic
    fun sendPlaylistEvent(action: PlaylistAction, param: Int) =
        sink?.sendPlaylistEvent(action, param) ?: Unit

    enum class VfsAction(val key: String) {
        REMOTE_FETCH("remote"), REMOTE_FALLBACK("remote_fallback"), CACHED_FETCH("cache"), CACHED_FALLBACK(
            "cache_fallback"
        ),
    }

    private fun sendVfsEvent(id: String, scope: String, action: VfsAction, duration: Long) =
        sink?.sendVfsEvent(id, scope, action, duration) ?: Unit

    @JvmStatic
    fun sendNoTracksFoundEvent(uri: Uri) = sink?.sendNoTracksFoundEvent(uri) ?: Unit

    @JvmStatic
    fun sendDbMetrics(name: String, size: Long, tablesRows: HashMap<String, Long>, duration: Long) =
        sink?.sendDbMetrics(name, size, tablesRows, duration) ?: Unit

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

    class VfsTrace private constructor(private val id: String, private val scope: String) {
        private val start = System.currentTimeMillis()

        fun send(action: VfsAction) {
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
