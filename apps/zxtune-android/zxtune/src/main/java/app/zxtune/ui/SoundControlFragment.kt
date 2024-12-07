package app.zxtune.ui

import android.app.Dialog
import android.os.Bundle
import android.os.ResultReceiver
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaControllerCompat
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.appcompat.app.AlertDialog
import androidx.core.os.bundleOf
import androidx.core.view.children
import androidx.core.view.isVisible
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.FragmentActivity
import androidx.fragment.app.activityViewModels
import androidx.lifecycle.lifecycleScope
import app.zxtune.MainService
import app.zxtune.R
import app.zxtune.analytics.Analytics
import app.zxtune.core.ModuleAttributes
import app.zxtune.core.Properties
import app.zxtune.device.media.MediaModel
import app.zxtune.ui.utils.await
import app.zxtune.ui.utils.combineTransformLatest
import app.zxtune.ui.utils.whenLifecycleStarted
import kotlinx.coroutines.launch
import kotlin.coroutines.resume
import kotlin.coroutines.suspendCoroutine

class SoundControlFragment : DialogFragment() {
    companion object {
        fun needShow(metadata: MediaMetadataCompat?) = metadata?.channelsNames != null

        fun show(activity: FragmentActivity) =
            SoundControlFragment().show(activity.supportFragmentManager, "sound")
    }

    private val model by activityViewModels<MediaModel>()

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog {
        val ctx = requireActivity()
        val view = ctx.layoutInflater.inflate(R.layout.sound_control, null)
        return AlertDialog.Builder(ctx).setTitle(R.string.sound_controls)
            .setIcon(R.drawable.ic_menu_sound_controls).setView(view).create().apply {
                whenLifecycleStarted {
                    combineTransformLatest(model.metadata, model.controller) { meta, ctrl ->
                        fillUi(view, meta?.channelsNames, ctrl)
                        emit(Unit)
                    }.collect {}
                }
            }
    }

    private suspend fun fillUi(
        view: View, channels: List<String>?, ctrl: MediaControllerCompat?
    ) {
        val channelsMuting = view.findViewById<ViewGroup>(R.id.channels_muting_group)
        channelsMuting.alphaTo(0f)
        val availableButtons = channelsMuting.childCount
        val requiredButtons = channels?.size ?: 0
        val inflater = requireActivity().layoutInflater
        for (idx in 0..<maxOf(availableButtons, requiredButtons)) {
            val button = (channelsMuting.getChildAt(idx) ?: inflater.inflate(
                R.layout.toggled_button, channelsMuting, false
            ).apply {
                channelsMuting.addView(this)
            }) as TextView
            button.apply {
                isVisible = idx < requiredButtons
                isEnabled = false
                isSelected = false
                text = channels?.getOrNull(idx)
            }
        }
        ctrl?.getCurrentParameters()?.let {
            val channelsCtrl = ChannelsControl(ctrl, it)
            channelsMuting.children.forEachIndexed { idx, view ->
                if (!view.isVisible) {
                    return@forEachIndexed
                }
                (view as TextView).apply {
                    setToggled(!channelsCtrl.isChannelMuted(idx))
                    setOnClickListener {
                        isEnabled = false
                        lifecycleScope.launch {
                            channelsCtrl.toggleChannelMute(idx)
                            setToggled(!channelsCtrl.isChannelMuted(idx))
                        }
                    }
                }
            }
            channelsMuting.alphaTo(1.0f)
        }
        Analytics.sendEvent("ui/soundcontrol", "channels" to requiredButtons)
    }
}

private fun TextView.setToggled(value: Boolean) {
    isSelected = value
    isEnabled = true
}

private val MediaMetadataCompat.channelsNames
    get() = getString(ModuleAttributes.CHANNELS_NAMES)?.split('\n')

private class ChannelsControl(
    private val ctrl: MediaControllerCompat, private val params: Bundle
) {
    private var currentMask = params.getInt(Properties.Core.CHANNELS_MASK, 0)

    fun isChannelMuted(idx: Int) = 0 != (currentMask and (1 shl idx))
    suspend fun toggleChannelMute(idx: Int) {
        val value = currentMask xor (1 shl idx)
        params.putInt(Properties.Core.CHANNELS_MASK, value)
        ctrl.setCurrentParameters(params)
        currentMask = value
    }
}

private suspend fun MediaControllerCompat.sendCommand(cmd: String, extra: Bundle) =
    suspendCoroutine { cont ->
        sendCommand(cmd, extra, object : ResultReceiver(null) {
            override fun onReceiveResult(resultCode: Int, resultData: Bundle?) {
                cont.resume(resultData)
            }
        })
    }

private suspend fun MediaControllerCompat.getCurrentParameters() = sendCommand(
    MainService.COMMAND_GET_CURRENT_PARAMETERS, bundleOf(Properties.Core.CHANNELS_MASK to 0)
)

private suspend fun MediaControllerCompat.setCurrentParameters(params: Bundle) = sendCommand(
    MainService.COMMAND_SET_CURRENT_PARAMETERS, params
)

private suspend fun View.alphaTo(alpha: Float) = animate().alpha(alpha).await()