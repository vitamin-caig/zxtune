package app.zxtune.ui

import android.content.ContentResolver
import android.support.v4.media.MediaMetadataCompat
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import androidx.core.view.MenuProvider
import androidx.fragment.app.Fragment
import app.zxtune.MainService
import app.zxtune.R
import app.zxtune.SharingActivity
import app.zxtune.device.media.MediaModel
import app.zxtune.ui.utils.enableIf
import app.zxtune.ui.utils.item

class TrackMenu(private val fragment: Fragment) : MenuProvider {
    private val activity
        get() = fragment.requireActivity()
    private val model
        get() = MediaModel.of(activity)

    override fun onCreateMenu(menu: Menu, menuInflater: MenuInflater) =
        menuInflater.inflate(R.menu.track, menu)

    override fun onPrepareMenu(menu: Menu) = menu.run {
        model.metadata.observe(fragment.viewLifecycleOwner) { metadata ->
            if (metadata == null) {
                return@observe
            }
            item(R.id.action_add).isEnabled = false == isFromProvider(metadata)
            item(R.id.action_send).enableIf =
                SharingActivity.maybeCreateSendIntent(activity, metadata)
            item(R.id.action_share).enableIf =
                SharingActivity.maybeCreateShareIntent(activity, metadata)
        }
    }

    override fun onMenuItemSelected(menuItem: MenuItem) = when (menuItem.itemId) {
        R.id.action_add -> {
            model.controller.value?.transportControls?.sendCustomAction(
                MainService.CUSTOM_ACTION_ADD_CURRENT,
                null
            )
            true
        }

        R.id.action_properties -> {
            model.metadata.value?.let {
                showPropertiesDialog(it)
            }
            true
        }

        R.id.action_make_ringtone -> {
            model.metadata.value?.description?.mediaUri?.let { fullId ->
                RingtoneFragment.show(activity, fullId)
            }
            true
        }

        else -> false
    }

    private fun showPropertiesDialog(meta: MediaMetadataCompat) =
        InformationFragment.show(activity, meta)
}

private fun isFromProvider(metadata: MediaMetadataCompat) =
    metadata.description.mediaId?.startsWith(ContentResolver.SCHEME_CONTENT)
