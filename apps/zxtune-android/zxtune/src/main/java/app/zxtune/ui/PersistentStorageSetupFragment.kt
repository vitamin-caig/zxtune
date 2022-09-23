package app.zxtune.ui

import android.app.Dialog
import android.content.Intent
import android.os.Bundle
import androidx.appcompat.app.AlertDialog
import androidx.fragment.app.DialogFragment
import app.zxtune.R

class PersistentStorageSetupFragment : DialogFragment() {

    override fun onCreateDialog(savedInstanceState: Bundle?): Dialog =
        AlertDialog.Builder(requireContext())
            .setPositiveButton(R.string.no_stored_playlists_access) { _, _ ->
                dismiss()
                val action = requireNotNull(arguments).getParcelable<Intent>(TAG_ACTION)
                startActivity(action)
            }
            .create()

    companion object {
        private const val TAG_ACTION = "action"

        @JvmStatic
        fun createInstance(action: Intent) = PersistentStorageSetupFragment().apply {
            arguments = Bundle().apply {
                putParcelable(TAG_ACTION, action)
            }
        }
    }
}
