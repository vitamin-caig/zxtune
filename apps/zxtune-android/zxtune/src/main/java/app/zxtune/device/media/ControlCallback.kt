package app.zxtune.device.media

import android.content.Context
import android.net.Uri
import android.os.Bundle
import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat.RepeatMode
import android.support.v4.media.session.PlaybackStateCompat.ShuffleMode
import app.zxtune.Logger
import app.zxtune.MainService
import app.zxtune.ScanService
import app.zxtune.TimeStamp.Companion.fromMilliseconds
import app.zxtune.playback.service.PlaybackServiceLocal
import app.zxtune.playback.stubs.PlayableItemStub

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
    }
}
