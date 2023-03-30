package app.zxtune.device.media

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.media.AudioManager
import app.zxtune.BroadcastReceiverConnection
import app.zxtune.Releaseable
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.PlaybackService

object NoisyAudioConnection {

    fun create(ctx: Context, ctrl: PlaybackControl): Releaseable {
        val broadcastReceiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context?, intent: Intent?) {
                if (AudioManager.ACTION_AUDIO_BECOMING_NOISY == intent?.action) {
                    ctrl.stop()
                }
            }
        }
        val filter = IntentFilter(AudioManager.ACTION_AUDIO_BECOMING_NOISY)
        return BroadcastReceiverConnection(ctx, broadcastReceiver, filter)
    }

    @JvmStatic
    fun create(ctx: Context, svc: PlaybackService) = create(ctx, svc.playbackControl)
}
