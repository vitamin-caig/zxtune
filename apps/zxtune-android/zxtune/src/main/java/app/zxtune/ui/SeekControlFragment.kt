/**
 * @file
 * @brief Seek control view logic
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.ui

import android.os.Bundle
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaControllerCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.SeekBar
import android.widget.SeekBar.OnSeekBarChangeListener
import android.widget.TextView
import androidx.fragment.app.Fragment
import app.zxtune.R
import app.zxtune.TimeStamp
import app.zxtune.device.media.MediaModel
import app.zxtune.device.media.getDuration
import app.zxtune.device.media.toMediaTime
import app.zxtune.ui.utils.UiUtils
import app.zxtune.ui.utils.whenLifecycleStarted
import kotlinx.coroutines.delay
import kotlinx.coroutines.launch

class SeekControlFragment : Fragment() {
    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ) = container?.let { inflater.inflate(R.layout.position, it, false) }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) =
        MediaModel.of(requireActivity()).run {
            val position = PositionControl(view)
            val looping = RepeatModeControl(view)

            viewLifecycleOwner.whenLifecycleStarted {
                launch {
                    controller.collect { controller ->
                        UiUtils.setViewEnabled(view, controller != null)
                        position.bindController(controller)
                        looping.bindController(controller)
                    }
                }
                launch {
                    metadata.collect(position::initTrack)
                }
                launch {
                    playbackPosition.collect { pos ->
                        position.update(pos)
                        delay(1000)
                    }
                }
            }
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
            metadata.getDuration().run {
                totalTime.text = toString()
                // TODO: use secondary progress to show loop
                currentPosition.max = toSeekBarItem()
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
                            transportControls.seekTo(fromSeekBarItem(progress).toMediaTime())
                        }
                    }

                    override fun onStartTrackingTouch(seekBar: SeekBar?) = Unit
                    override fun onStopTrackingTouch(seekBar: SeekBar?) = Unit
                }
            })
        }

        fun update(pos: TimeStamp?) {
            currentPosition.isEnabled = pos != null
            if (pos != null) {
                currentPosition.progress = pos.toSeekBarItem()
                currentTime.text = pos.toString()
            } else {
                currentTime.setText(R.string.stub_time)
            }
        }
    }

    private class RepeatModeControl(view: View) {
        private var button = view.findViewById<View>(R.id.controls_track_mode)
        private var repeatMode = PlaybackStateCompat.REPEAT_MODE_INVALID
            set(value) {
                field = value
                button.run {
                    isEnabled = value != PlaybackStateCompat.REPEAT_MODE_INVALID
                    isActivated = value == PlaybackStateCompat.REPEAT_MODE_ONE
                }
            }

        init {
            bindController(null)
        }

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
            PlaybackStateCompat.REPEAT_MODE_NONE -> PlaybackStateCompat.REPEAT_MODE_ONE
            PlaybackStateCompat.REPEAT_MODE_ONE -> PlaybackStateCompat.REPEAT_MODE_NONE
            else -> null
        }
    }
}

private fun TimeStamp.toSeekBarItem() = toSeconds().toInt()
private fun fromSeekBarItem(pos: Int) = TimeStamp.fromSeconds(pos.toLong())
