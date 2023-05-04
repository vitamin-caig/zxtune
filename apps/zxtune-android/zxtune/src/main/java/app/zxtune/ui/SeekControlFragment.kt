/**
 * @file
 * @brief Seek control view logic
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.ui

import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.MediaMetadataCompat.METADATA_KEY_DURATION
import android.support.v4.media.session.MediaControllerCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.SeekBar
import android.widget.SeekBar.OnSeekBarChangeListener
import android.widget.TextView
import androidx.fragment.app.Fragment
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.LifecycleOwner
import app.zxtune.Logger
import app.zxtune.R
import app.zxtune.TimeStamp
import app.zxtune.device.media.MediaModel
import app.zxtune.playback.PlaybackControl
import app.zxtune.playback.PlaybackControl.TrackMode
import app.zxtune.ui.utils.UiUtils

class SeekControlFragment : Fragment() {
    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ) = container?.let { inflater.inflate(R.layout.position, it, false) }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) =
        MediaModel.of(requireActivity()).run {
            val position = PositionControl(view)
            val looping = RepeatModeControl(view)
            val updating = StateUpdate(position)

            controller.observe(viewLifecycleOwner) { controller ->
                UiUtils.setViewEnabled(view, controller != null)
                position.bindController(controller)
                looping.bindController(controller)
            }
            metadata.observe(viewLifecycleOwner, position::initTrack)
            playbackState.observe(viewLifecycleOwner, updating::update)
            viewLifecycleOwner.lifecycle.addObserver(object : DefaultLifecycleObserver {
                override fun onStop(owner: LifecycleOwner) {
                    updating.update(null)
                }
            })
        }

    private class PositionControl(view: View) {
        private val currentTime = view.findViewById<TextView>(R.id.position_time)
        private val currentPosition = view.findViewById<SeekBar>(R.id.position_seek)
        private val totalTime = view.findViewById<TextView>(R.id.position_duration)

        init {
            initTrack(null)
            bindController(null)
        }

        fun initTrack(metadata: MediaMetadataCompat?) = if (metadata != null) {
            TimeStamp.fromMilliseconds(metadata.getLong(METADATA_KEY_DURATION)).run {
                totalTime.text = toString()
                // TODO: use secondary progress to show loop
                currentPosition.max = toSeconds().toInt()
            }
        } else {
            totalTime.setText(R.string.stub_time)
            currentTime.setText(R.string.stub_time)
        }

        fun bindController(ctrl: MediaControllerCompat?) {
            currentPosition.setOnSeekBarChangeListener(ctrl?.run {
                object : OnSeekBarChangeListener {
                    override fun onProgressChanged(
                        seekBar: SeekBar?, progress: Int, fromUser: Boolean
                    ) {
                        if (fromUser) {
                            transportControls.seekTo(progress * 1000L)
                        }
                    }

                    override fun onStartTrackingTouch(seekBar: SeekBar?) = Unit
                    override fun onStopTrackingTouch(seekBar: SeekBar?) = Unit
                }
            })
        }

        fun update(pos: TimeStamp) {
            currentPosition.progress = pos.toSeconds().toInt()
            currentTime.text = pos.toString()
        }
    }

    private class RepeatModeControl(view: View) {
        private var button = view.findViewById<ImageView>(R.id.controls_track_mode)
        private var repeatMode = PlaybackStateCompat.REPEAT_MODE_INVALID
            set(value) {
                field = value
                button.run {
                    isEnabled = value != PlaybackStateCompat.REPEAT_MODE_INVALID
                    setImageResource(
                        when (value) {
                            TrackMode.LOOPED.ordinal -> R.drawable.ic_track_looped
                            else -> R.drawable.ic_track_regular
                        }
                    )
                }
            }

        init {
            bindController(null)
        }

        // TODO: use PlaybackStateCompat.REPEAT_MODE_*
        fun bindController(controller: MediaControllerCompat?) {
            repeatMode = controller?.repeatMode ?: PlaybackStateCompat.REPEAT_MODE_INVALID
            button.setOnClickListener(controller?.transportControls?.let { transport ->
                {
                    getNextMode()?.let { newMode ->
                        transport.setRepeatMode(newMode)
                        repeatMode = newMode
                    }
                }
            })
        }

        private fun getNextMode() = when (repeatMode) {
            TrackMode.LOOPED.ordinal, TrackMode.REGULAR.ordinal -> (repeatMode + 1) % PlaybackControl.TrackMode.values().size
            else -> null
        }
    }

    private class StateUpdate(pos: PositionControl) {
        private var posMs = 0L
        private var speed = 0f
        private var ts = 0L
        private val handler = Handler(Looper.getMainLooper())
        private val task = object : Runnable {
            override fun run() {
                try {
                    val now = SystemClock.elapsedRealtime()
                    val newPosMs = posMs + ((now - ts) * speed).toLong()
                    pos.update(TimeStamp.fromMilliseconds(newPosMs))
                    handler.postDelayed(this, 1000)
                } catch (e: Exception) {
                    LOG.w(e) { "updateViewTask.run()" }
                }
            }
        }

        // TODO: also process seeking at StatusCallback.sendState
        fun update(state: PlaybackStateCompat?) =
            state?.takeIf { it.state == PlaybackStateCompat.STATE_PLAYING }?.run {
                posMs = position
                speed = playbackSpeed
                ts = lastPositionUpdateTime
                task.run()
            } ?: run {
                handler.removeCallbacks(task)
            }
    }

    companion object {
        private val LOG = Logger(SeekControlFragment::class.java.name)
    }
}
