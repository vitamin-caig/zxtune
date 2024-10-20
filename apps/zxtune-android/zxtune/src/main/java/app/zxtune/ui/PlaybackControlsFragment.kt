/**
 * @file
 * @brief Playback controls logic
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.ui

import android.os.Bundle
import android.support.v4.media.session.MediaControllerCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageButton
import android.widget.ImageView
import androidx.appcompat.widget.PopupMenu
import androidx.fragment.app.Fragment
import app.zxtune.R
import app.zxtune.device.media.MediaModel
import app.zxtune.ui.utils.UiUtils

class PlaybackControlsFragment : Fragment() {

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ) = container?.let {
        inflater.inflate(R.layout.controls, it, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) =
        MediaModel.of(requireActivity()).run {
            val actions = Actions(view)
            val shuffle = ShuffleModeControl(view)
            controller.observe(viewLifecycleOwner) { controller ->
                UiUtils.setViewEnabled(view, controller != null)
                actions.bindController(controller)
                shuffle.bindController(controller)
            }
            playbackState.observe(viewLifecycleOwner, actions::bindState)
            TrackContextMenu(this@PlaybackControlsFragment).install(view.findViewById(R.id.controls_track_menu))
        }

    private class Actions(view: View) {
        private val prev = view.findViewById<View>(R.id.controls_prev)
        private val playPause = view.findViewById<ImageView>(R.id.controls_play_pause)
        private val next = view.findViewById<View>(R.id.controls_next)
        private var state = PlaybackStateCompat.STATE_NONE
            set(value) {
                field = value
                playPause.run {
                    isEnabled = state != PlaybackStateCompat.STATE_NONE
                    setImageResource(
                        if (state == PlaybackStateCompat.STATE_PLAYING) {
                            R.drawable.ic_pause
                        } else {
                            R.drawable.ic_play
                        }
                    )
                }
            }

        init {
            bindController(null)
            bindState(null)
        }

        fun bindController(ctrl: MediaControllerCompat?) = ctrl?.transportControls?.run {
            prev.setOnClickListener { skipToPrevious() }
            next.setOnClickListener { skipToNext() }
            playPause.setOnClickListener {
                when (state) {
                    PlaybackStateCompat.STATE_PLAYING -> stop()
                    PlaybackStateCompat.STATE_STOPPED -> play()
                    else -> Unit
                }
            }
        } ?: run {
            prev.setOnClickListener(null)
            next.setOnClickListener(null)
            playPause.setOnClickListener(null)
        }

        fun bindState(playbackState: PlaybackStateCompat?) {
            state = playbackState?.state ?: PlaybackStateCompat.STATE_NONE
        }
    }

    private class ShuffleModeControl(view: View) {
        private val button = view.findViewById<View>(R.id.controls_sequence_mode)
        private var shuffleMode = PlaybackStateCompat.SHUFFLE_MODE_INVALID
            set(value) {
                field = value
                button.run {
                    isEnabled = value != PlaybackStateCompat.SHUFFLE_MODE_INVALID
                    isActivated = value == PlaybackStateCompat.SHUFFLE_MODE_ALL
                }
            }

        init {
            bindController(null)
        }

        fun bindController(ctrl: MediaControllerCompat?) {
            shuffleMode = ctrl?.shuffleMode ?: PlaybackStateCompat.SHUFFLE_MODE_INVALID
            button.setOnClickListener(ctrl?.transportControls?.let { transport ->
                {
                    getNextMode()?.let { newMode ->
                        transport.setShuffleMode(newMode)
                        shuffleMode = newMode
                    }
                }
            })
        }

        private fun getNextMode() = when (shuffleMode) {
            PlaybackStateCompat.SHUFFLE_MODE_NONE -> PlaybackStateCompat.SHUFFLE_MODE_ALL
            PlaybackStateCompat.SHUFFLE_MODE_ALL -> PlaybackStateCompat.SHUFFLE_MODE_NONE
            else -> null
        }
    }

    private class TrackContextMenu(fragment: Fragment) {
        private val provider by lazy {
            TrackMenu(fragment)
        }
        private lateinit var popup: PopupMenu

        fun install(view: View) = view.setOnClickListener {
            if (!this::popup.isInitialized) {
                popup = PopupMenu(view.context, view).apply {
                    provider.run {
                        onCreateMenu(menu, menuInflater)
                        onPrepareMenu(menu)
                        setOnMenuItemClickListener(this::onMenuItemSelected)
                    }
                    setForceShowIcon(true)
                }
            }
            popup.show()
        }
    }
}
