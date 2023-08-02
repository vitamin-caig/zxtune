package app.zxtune.ui

import android.content.Intent
import android.net.Uri
import android.os.Bundle
import android.provider.Settings
import android.view.View
import android.view.ViewGroup
import androidx.activity.result.contract.ActivityResultContracts
import androidx.annotation.RequiresApi
import androidx.core.view.children
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.FragmentActivity
import app.zxtune.R
import app.zxtune.RingtoneService
import app.zxtune.TimeStamp
import app.zxtune.device.Permission
import app.zxtune.ui.utils.FragmentParcelableProperty

class RingtoneFragment : DialogFragment(R.layout.ringtone) {
    companion object {
        fun show(activity: FragmentActivity, uri: Uri) = RingtoneFragment().apply {
            this.uri = uri
        }.show(activity.supportFragmentManager, null)
    }

    private var uri by FragmentParcelableProperty<Uri>()

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) {
        super.onViewCreated(view, savedInstanceState)
        fillUi()
        if (!Permission.canWriteSystemSettings(requireContext())) {
            requestPermission()
        }
    }

    override fun onCreateDialog(savedInstanceState: Bundle?) =
        super.onCreateDialog(savedInstanceState).apply {
            setTitle(R.string.ringtone_create_title)
        }


    private fun fillUi() {
        val listener = View.OnClickListener { v: View ->
            val tagValue = (v.tag as String).toInt()
            RingtoneService.execute(requireContext(), uri, TimeStamp.fromSeconds(tagValue.toLong()))
            dismiss()
        }
        requireNotNull(view?.findViewById<ViewGroup>(R.id.ringtone_durations)).children.forEach {
            it.setOnClickListener(listener)
        }
    }

    @RequiresApi(23)
    private fun requestPermission() =
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
            if (!Permission.canWriteSystemSettings(requireContext())) {
                dismiss()
            }
        }.launch(
            Intent(
                Settings.ACTION_MANAGE_WRITE_SETTINGS,
                Uri.parse("package:${requireContext().packageName}")
            )
        )
}
