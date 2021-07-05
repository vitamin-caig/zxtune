package app.zxtune.playback.service

import app.zxtune.Logger
import app.zxtune.TimeStamp
import app.zxtune.playback.Callback
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.SeekControl
import app.zxtune.sound.PlayerEventsListener

private val LOG = Logger(PlaybackEvents::class.java.name)

internal class PlaybackEvents(
    private val callback: Callback,
    private val ctrl: PlaybackControl,
    private val seek: SeekControl
) : PlayerEventsListener {
    override fun onStart() = callback.onStateChanged(PlaybackControl.State.PLAYING, pos)

    override fun onSeeking() = callback.onStateChanged(PlaybackControl.State.SEEKING, pos)

    override fun onFinish() = try {
        ctrl.next()
    } catch (e: Exception) {
        onError(e)
    }

    override fun onStop() = callback.onStateChanged(PlaybackControl.State.STOPPED, pos)

    override fun onError(e: Exception) = LOG.w(e) { "Error occurred" }

    private val pos: TimeStamp
        get() = try {
            seek.position
        } catch (e: Exception) {
            TimeStamp.EMPTY
        }
}
