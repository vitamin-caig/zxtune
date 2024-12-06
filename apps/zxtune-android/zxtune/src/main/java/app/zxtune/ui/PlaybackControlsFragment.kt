/**
 * @file
 * @brief Playback controls logic
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.ui

import android.content.ContentResolver
import android.os.Bundle
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaControllerCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.view.LayoutInflater
import android.view.Menu
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import androidx.appcompat.widget.PopupMenu
import androidx.fragment.app.Fragment
import androidx.fragment.app.FragmentActivity
import app.zxtune.MainService
import app.zxtune.R
import app.zxtune.SharingActivity
import app.zxtune.device.media.MediaModel
import app.zxtune.ui.utils.UiUtils
import app.zxtune.ui.utils.bindIntent
import app.zxtune.ui.utils.bindOnClick
import app.zxtune.ui.utils.item
import app.zxtune.ui.utils.whenLifecycleStarted
import app.zxtune.utils.ifNotNulls
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.launch

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
            val menu =
                TrackContextMenu(requireActivity(), view.findViewById(R.id.controls_track_menu))
            viewLifecycleOwner.whenLifecycleStarted {
                launch {
                    controller.collect { controller ->
                        UiUtils.setViewEnabled(view, controller != null)
                        actions.bindController(controller)
                        shuffle.bindController(controller)
                    }
                }
                launch {
                    playbackState.collect(actions::bindState)
                }
                launch {
                    combine(controller, metadata) { ctrl, meta ->
                        menu.bind(ctrl, meta)
                    }.collect {}
                }
            }
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

    private class TrackContextMenu(private val ctx: FragmentActivity, anchor: View) {
        private val popup by lazy {
            PopupMenu(anchor.context, anchor).apply {
                menuInflater.inflate(R.menu.track, menu)
                setForceShowIcon(true)
            }
        }
        private var setupItems: (Menu) -> Unit = { menu -> setup(menu, null, null) }

        init {
            anchor.setOnClickListener {
                popup.apply {
                    setupItems(menu)
                }.show()
            }
        }

        fun bind(controller: MediaControllerCompat?, metadata: MediaMetadataCompat?) {
            setupItems = { menu -> setup(menu, controller, metadata) }
        }

        private fun setup(
            menu: Menu,
            controller: MediaControllerCompat?,
            metadata: MediaMetadataCompat?,
        ) = with(menu) {
            item(R.id.action_add).bindOnClick(ifNotNulls(controller?.transportControls,
                metadata?.description?.mediaId?.takeUnless {
                    it.startsWith(ContentResolver.SCHEME_CONTENT)
                }) { ctrl, _ ->
                {
                    ctrl.sendCustomAction(MainService.CUSTOM_ACTION_ADD_CURRENT, null)
                }
            })
            item(R.id.action_send).bindIntent(metadata?.let {
                SharingActivity.maybeCreateSendIntent(
                    ctx, it
                )
            })
            item(R.id.action_share).bindIntent(metadata?.let {
                SharingActivity.maybeCreateShareIntent(
                    ctx, it
                )
            })
            item(R.id.action_make_ringtone).bindOnClick(metadata?.description?.mediaUri?.let {
                { RingtoneFragment.show(ctx, it) }
            })
            item(R.id.action_properties).bindOnClick(metadata?.let {
                { InformationFragment.show(ctx, it) }
            })
            item(R.id.action_sound_controls).bindOnClick(if (SoundControlFragment.needShow(metadata)) {
                { SoundControlFragment.show(ctx) }
            } else null)
        }
    }
}
