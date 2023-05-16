package app.zxtune.device.media

import android.support.v4.media.session.PlaybackStateCompat
import app.zxtune.playback.PlaybackControl

@PlaybackStateCompat.State
internal fun PlaybackControl.State.toState() = when (this) {
    PlaybackControl.State.PLAYING -> PlaybackStateCompat.STATE_PLAYING
    PlaybackControl.State.SEEKING -> PlaybackStateCompat.STATE_BUFFERING //?
    PlaybackControl.State.STOPPED -> PlaybackStateCompat.STATE_STOPPED
}

@PlaybackStateCompat.ShuffleMode
internal fun PlaybackControl.SequenceMode.toShuffleMode() = when (this) {
    PlaybackControl.SequenceMode.ORDERED -> PlaybackStateCompat.SHUFFLE_MODE_NONE
    PlaybackControl.SequenceMode.SHUFFLE -> PlaybackStateCompat.SHUFFLE_MODE_ALL
    PlaybackControl.SequenceMode.LOOPED -> PlaybackStateCompat.SHUFFLE_MODE_GROUP
}

internal fun fromShuffleMode(@PlaybackStateCompat.ShuffleMode mode: Int) = when (mode) {
    PlaybackStateCompat.SHUFFLE_MODE_NONE -> PlaybackControl.SequenceMode.ORDERED
    PlaybackStateCompat.SHUFFLE_MODE_ALL -> PlaybackControl.SequenceMode.SHUFFLE
    PlaybackStateCompat.SHUFFLE_MODE_GROUP -> PlaybackControl.SequenceMode.LOOPED
    else -> null
}

@PlaybackStateCompat.RepeatMode
internal fun PlaybackControl.TrackMode.toRepeatMode() = when (this) {
    PlaybackControl.TrackMode.REGULAR -> PlaybackStateCompat.REPEAT_MODE_NONE
    PlaybackControl.TrackMode.LOOPED -> PlaybackStateCompat.REPEAT_MODE_ONE
}

internal fun fromRepeatMode(@PlaybackStateCompat.RepeatMode mode: Int) = when (mode) {
    PlaybackStateCompat.REPEAT_MODE_NONE -> PlaybackControl.TrackMode.REGULAR
    PlaybackStateCompat.REPEAT_MODE_ONE -> PlaybackControl.TrackMode.LOOPED
    else -> null
}
