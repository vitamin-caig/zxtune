package app.zxtune.ui

import android.app.Dialog
import android.content.Intent
import android.os.Bundle
import androidx.appcompat.app.AlertDialog
import androidx.fragment.app.DialogFragment
import app.zxtune.R
import app.zxtune.ui.utils.FragmentParcelableProperty

class PersistentStorageSetupFragment : DialogFragment() {
    private var action by FragmentParcelableProperty<Intent>()

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog =
        AlertDialog.Builder(requireContext())
            .setPositiveButton(R.string.no_stored_playlists_access) { _, _ ->
                dismiss()
                startActivity(action)
            }.create()

    companion object {
        fun createInstance(action: Intent) = PersistentStorageSetupFragment().apply {
            this.action = action
        }
    }
}
