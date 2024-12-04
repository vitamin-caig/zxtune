package app.zxtune.device.media

import android.content.Context
import android.net.Uri
import android.os.Bundle
import android.os.ResultReceiver
import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat.RepeatMode
import android.support.v4.media.session.PlaybackStateCompat.ShuffleMode
import app.zxtune.Logger
import app.zxtune.MainService
import app.zxtune.ScanService
import app.zxtune.TimeStamp.Companion.fromMilliseconds
import app.zxtune.core.PropertiesAccessor
import app.zxtune.core.PropertiesModifier
import app.zxtune.playback.service.PlaybackServiceLocal
import app.zxtune.playback.stubs.PlayableItemStub
import app.zxtune.preferences.RawPropertiesAdapter
import app.zxtune.utils.ifNotNulls

internal class ControlCallback(
    private val ctx: Context,
    private val svc: PlaybackServiceLocal,
    private val session: MediaSessionCompat,
) : MediaSessionCompat.Callback() {
    private val ctrl
        get() = svc.playbackControl
    private val seek
        get() = svc.seekControl

    override fun onPlay() = ctrl.play()

    override fun onPause() = onStop()

    override fun onStop() = ctrl.stop()

    override fun onSkipToPrevious() = ctrl.prev()

    override fun onSkipToNext() = ctrl.next()

    override fun onSeekTo(ms: Long) {
        try {
            seek.position = fromMilliseconds(ms)
        } catch (e: Exception) {
            LOG.w(e) { "Failed to seek" }
        }
    }

    override fun onCustomAction(action: String, extra: Bundle?) = when (action) {
        MainService.CUSTOM_ACTION_ADD_CURRENT -> addCurrent()
        MainService.CUSTOM_ACTION_ADD -> extra?.getParcelableArray("uris")?.let {
            ScanService.add(ctx, Array(it.size) { idx -> it[idx] as Uri })
        } ?: Unit

        else -> Unit
    }

    override fun onCommand(command: String?, extras: Bundle?, cb: ResultReceiver?) =
        when (command) {
            MainService.COMMAND_SET_CURRENT_PARAMETERS -> ifNotNulls(
                extras, svc.playbackProperties
            ) { data, props ->
                setProperties(data, props)
                cb?.send(0, data)
            } ?: cb?.send(-1, null) ?: Unit

            MainService.COMMAND_GET_CURRENT_PARAMETERS -> ifNotNulls(
                extras, svc.playbackProperties, cb
            ) { query, props, receiver ->
                getProperties(props, query)
                receiver.send(0, query)
            } ?: cb?.send(-1, null) ?: Unit

            else -> Unit
        }

    private fun addCurrent() = svc.nowPlaying.takeIf { it !== PlayableItemStub.instance() }?.let {
        ScanService.add(ctx, it)
    } ?: Unit

    override fun onSetShuffleMode(@ShuffleMode mode: Int) = fromShuffleMode(mode)?.let {
        ctrl.sequenceMode = it
        session.setShuffleMode(mode)
    } ?: Unit

    override fun onSetRepeatMode(@RepeatMode mode: Int) = fromRepeatMode(mode)?.let {
        ctrl.trackMode = it
        session.setRepeatMode(mode)
    } ?: Unit

    override fun onPlayFromUri(uri: Uri, extras: Bundle?) = svc.setNowPlaying(uri)

    companion object {
        private val LOG = Logger(ControlCallback::class.java.name)

        private fun setProperties(src: Bundle, props: PropertiesModifier) =
            with(RawPropertiesAdapter(props)) {
                src.keySet().forEach { key ->
                    src[key]?.let {
                        LOG.d { "set prop[$key]=$it" }
                        setProperty(key, it)
                    }
                }
            }

        private fun getProperties(src: PropertiesAccessor, data: Bundle) =
            data.keySet().forEach { key ->
                copyProperty(src, key, data)
            }

        private fun copyProperty(src: PropertiesAccessor, key: String, data: Bundle) =
            when (val obj = data[key]) {
                is String -> src.getProperty(key, obj).let {
                    LOG.d { "get prop[$key, $obj]=$it" }
                    data.putString(key, it)
                }

                is Long -> src.getProperty(key, obj).let {
                    LOG.d { "get prop[$key, $obj]=$it" }
                    data.putLong(key, it)
                }

                is Int -> src.getProperty(key, obj.toLong()).let {
                    LOG.d { "get prop[$key, $obj]=$it" }
                    data.putInt(key, it.toInt())
                }

                else -> Unit
            }
    }
}
