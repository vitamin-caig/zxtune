package app.zxtune.device.media

import android.content.Context
import android.media.AudioManager
import android.media.AudioManager.OnAudioFocusChangeListener
import androidx.annotation.VisibleForTesting
import androidx.core.content.getSystemService
import app.zxtune.Logger
import app.zxtune.Releaseable
import app.zxtune.TimeStamp
import app.zxtune.device.sound.SoundOutputSamplesTarget
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.PlaybackService
import app.zxtune.playback.stubs.CallbackStub

private val LOG = Logger(AudioFocusConnection::class.java.name)

class AudioFocusConnection @VisibleForTesting constructor(
    private val manager: AudioManager,
    svc: PlaybackService,
) : Releaseable {

    private val ctrl = svc.playbackControl
    private val stateConnection = svc.subscribe(object : CallbackStub() {
        override fun onStateChanged(state: PlaybackControl.State, pos: TimeStamp) = when (state) {
            PlaybackControl.State.PLAYING -> onPlaying()
            PlaybackControl.State.STOPPED -> onStopped()
            else -> Unit
        }
    })
    private val focusListener = OnAudioFocusChangeListener { focusChange ->
        when (focusChange) {
            AudioManager.AUDIOFOCUS_LOSS, AudioManager.AUDIOFOCUS_LOSS_TRANSIENT -> {
                LOG.d { "Focus lost" }
                onFocusLost()
            }
            AudioManager.AUDIOFOCUS_GAIN -> {
                LOG.d { "Focus restored" }
                onFocusRestore()
            }
        }
    }
    private var focusConnection: Releaseable? = null
    private var lostFocus = false

    override fun release() {
        releaseFocus()
        stateConnection.release()
    }

    private fun gainFocus() =
        if (AudioManager.AUDIOFOCUS_REQUEST_FAILED != manager.requestAudioFocus(
                focusListener, SoundOutputSamplesTarget.STREAM, AudioManager.AUDIOFOCUS_GAIN
            )
        ) {
            focusConnection = Releaseable { manager.abandonAudioFocus(focusListener) }
            lostFocus = false
            true
        } else {
            false
        }

    private fun releaseFocus() {
        focusConnection?.release()
        focusConnection = null
    }

    private fun onPlaying() {
        if (lostFocus) {
            releaseFocus()
        }
        if (focusConnection == null && !gainFocus()) {
            LOG.d { "Failed to gain focus" }
            ctrl.stop()
        }
    }

    private fun onStopped() {
        if (!lostFocus) {
            releaseFocus()
        }
    }

    private fun onFocusLost() {
        lostFocus = true
        ctrl.stop()
    }

    private fun onFocusRestore() {
        lostFocus = false
        ctrl.play()
    }

    companion object {
        @JvmStatic
        fun create(ctx: Context, svc: PlaybackService): Releaseable =
            AudioFocusConnection(requireNotNull(ctx.getSystemService()), svc)
    }
}
