package app.zxtune.analytics

import android.content.Context
import android.net.Uri
import androidx.collection.LongSparseArray
import androidx.collection.size
import app.zxtune.Logger
import app.zxtune.analytics.Analytics.BrowserAction
import app.zxtune.analytics.Analytics.PlaylistAction
import app.zxtune.analytics.Analytics.SocialAction
import app.zxtune.analytics.Analytics.UiAction
import app.zxtune.analytics.Analytics.VfsAction
import app.zxtune.analytics.internal.Factory.createClientEndpoint
import app.zxtune.analytics.internal.UrlsBuilder
import app.zxtune.core.ModuleAttributes
import app.zxtune.core.Player
import app.zxtune.playback.PlayableItem
import app.zxtune.playlist.ProviderClient
import app.zxtune.playlist.ProviderClient.SortBy

internal class InternalSink(ctx: Context) : Sink {
    private val delegate = createClientEndpoint(ctx)

    override fun logException(e: Throwable) = Unit

    override fun sendTrace(id: String, points: LongSparseArray<String>) =
        UrlsBuilder("trace").apply {
            addParam("id", id)
            for (idx in 0..<points.size) {
                val tag = points.valueAt(idx)
                val offset = points.keyAt(idx)
                addParam(tag, offset)
            }
        }.let {
            send(it)
        }

    override fun sendPlayEvent(item: PlayableItem, player: Player) =
        UrlsBuilder("track/done").apply {
            item.run {
                addUri(dataId.fullLocation)
                addParam("from", id.scheme)
                addParam("duration", duration.toSeconds())
            }
            item.module.run {
                addParam(
                    "type",
                    getProperty(ModuleAttributes.TYPE, UrlsBuilder.DEFAULT_STRING_VALUE)
                )
                addParam(
                    "container",
                    getProperty(ModuleAttributes.CONTAINER, UrlsBuilder.DEFAULT_STRING_VALUE)
                )

                addParam("crc", getProperty("CRC", UrlsBuilder.DEFAULT_LONG_VALUE))
                addParam("fixedcrc", getProperty("FixedCRC", UrlsBuilder.DEFAULT_LONG_VALUE))
                addParam("size", getProperty("Size", UrlsBuilder.DEFAULT_LONG_VALUE))
            }
            player.run {
                addParam("perf", performance.toLong())
                addParam("progress", progress.toLong())
            }
        }.let {
            send(it)
        }

    override fun sendBrowserEvent(path: Uri, @BrowserAction action: Int) =
        UrlsBuilder("ui/browser/${serializeBrowserAction(action)}").apply {
            addUri(path)
        }.let {
            send(it)
        }

    override fun sendSocialEvent(path: Uri, app: String, @SocialAction action: Int) =
        UrlsBuilder("ui/${serializeSocialAction(action)}").apply {
            addUri(path)
        }.let {
            send(it)
        }

    override fun sendUiEvent(@UiAction action: Int) =
        send(UrlsBuilder("ui/${serializeUiAction(action)}"))

    override fun sendPlaylistEvent(@PlaylistAction action: Int, param: Int) =
        UrlsBuilder("ui/playlist/${serializePlaylistAction(action)}").apply {
            if (action == Analytics.PLAYLIST_ACTION_SORT) {
                addParam("by", SortBy.entries.toTypedArray()[param / 100].name)
                addParam("order", ProviderClient.SortOrder.entries.toTypedArray()[param % 100].name)
            } else {
                addParam(
                    "count",
                    if (param != 0) param.toLong() else UrlsBuilder.DEFAULT_LONG_VALUE
                )
            }
        }.let {
            send(it)
        }

    override fun sendVfsEvent(
        id: String, scope: String, @VfsAction action: Int,
        duration: Long
    ) = UrlsBuilder("vfs/${serializeVfsAction(action)}").apply {
        addParam("id", id)
        addParam("scope", scope)
        addParam("duration", duration)
    }.let {
        send(it)
    }

    override fun sendNoTracksFoundEvent(uri: Uri) = UrlsBuilder("track/notfound").apply {
        addUri(uri)
        if ("file" == uri.scheme) {
            val filename = uri.lastPathSegment
            val extPos = filename?.lastIndexOf('.') ?: -1
            val type = if (extPos != -1) filename!!.substring(extPos + 1) else "none"
            addParam("type", type)
        }
    }.let {
        send(it)
    }

    override fun sendDbMetrics(name: String, size: Long, tablesRows: HashMap<String, Long>) =
        UrlsBuilder("db/stat").apply {
            addParam("name", name)
            addParam("size", size)
            for ((key, value) in tablesRows) {
                addParam("rows_$key", value)
            }
        }.let {
            send(it)
        }

    override fun sendEvent(id: String, vararg arguments: Pair<String, *>) = UrlsBuilder(id).apply {
        for ((key, second) in arguments) {
            when (second) {
                is String -> addParam(key, second)
                is Long -> addParam(key, second)
                is Int -> addParam(key, second.toLong())
                is Boolean -> addParam(key, if (second) 1L else 0L)
                is Uri -> addUri(second)
                else -> LOG.d { "Unexpected event argument ${key}=${second}" }
            }
        }
    }.let {
        send(it)
    }

    private fun send(builder: UrlsBuilder) = try {
        delegate.push(builder.result)
    } catch (e: Exception) {
        LOG.w(e) { "Failed to send event" }
    }


    companion object {
        private val LOG = Logger(InternalSink::class.java.name)

        private fun serializeBrowserAction(@BrowserAction action: Int): String {
            return when (action) {
                Analytics.BROWSER_ACTION_BROWSE -> "browse"
                Analytics.BROWSER_ACTION_BROWSE_PARENT -> "up"
                Analytics.BROWSER_ACTION_SEARCH -> "search"
                else -> ""
            }
        }

        private fun serializeSocialAction(@SocialAction action: Int): String {
            return when (action) {
                Analytics.SOCIAL_ACTION_RINGTONE -> "ringtone"
                Analytics.SOCIAL_ACTION_SHARE -> "share"
                Analytics.SOCIAL_ACTION_SEND -> "send"
                else -> ""
            }
        }

        private fun serializeUiAction(@UiAction action: Int): String {
            return when (action) {
                Analytics.UI_ACTION_OPEN -> "open"
                Analytics.UI_ACTION_CLOSE -> "close"
                Analytics.UI_ACTION_PREFERENCES -> "preferences"
                Analytics.UI_ACTION_RATE -> "rate"
                Analytics.UI_ACTION_ABOUT -> "about"
                Analytics.UI_ACTION_QUIT -> "quit"
                else -> ""
            }
        }

        private fun serializePlaylistAction(@PlaylistAction action: Int): String {
            return when (action) {
                Analytics.PLAYLIST_ACTION_ADD -> "add"
                Analytics.PLAYLIST_ACTION_DELETE -> "delete"
                Analytics.PLAYLIST_ACTION_MOVE -> "move"
                Analytics.PLAYLIST_ACTION_SORT -> "sort"
                Analytics.PLAYLIST_ACTION_SAVE -> "save"
                Analytics.PLAYLIST_ACTION_STATISTICS -> "statistics"
                else -> ""
            }
        }

        private fun serializeVfsAction(@VfsAction action: Int): String {
            return when (action) {
                Analytics.VFS_ACTION_REMOTE_FETCH -> "remote"
                Analytics.VFS_ACTION_REMOTE_FALLBACK -> "remote_fallback"
                Analytics.VFS_ACTION_CACHED_FETCH -> "cache"
                Analytics.VFS_ACTION_CACHED_FALLBACK -> "cache_fallback"
                else -> ""
            }
        }
    }
}
