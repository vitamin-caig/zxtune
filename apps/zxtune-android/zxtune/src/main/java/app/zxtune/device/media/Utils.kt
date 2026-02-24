package app.zxtune.device.media

import android.support.v4.media.session.PlaybackStateCompat
import app.zxtune.playback.PlaybackService

@PlaybackStateCompat.State
internal fun PlaybackService.State.toState() = when (this) {
    PlaybackService.State.PLAYING -> PlaybackStateCompat.STATE_PLAYING
    PlaybackService.State.SEEKING -> PlaybackStateCompat.STATE_BUFFERING //?
    PlaybackService.State.STOPPED -> PlaybackStateCompat.STATE_STOPPED
}
