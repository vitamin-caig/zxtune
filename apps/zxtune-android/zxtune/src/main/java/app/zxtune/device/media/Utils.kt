package app.zxtune.device.media

import android.support.v4.media.session.PlaybackStateCompat
import app.zxtune.playback.PlaybackControl

@PlaybackStateCompat.State
internal fun PlaybackControl.State.toState() = when (this) {
    PlaybackControl.State.PLAYING -> PlaybackStateCompat.STATE_PLAYING
    PlaybackControl.State.SEEKING -> PlaybackStateCompat.STATE_BUFFERING //?
    PlaybackControl.State.STOPPED -> PlaybackStateCompat.STATE_STOPPED
}
