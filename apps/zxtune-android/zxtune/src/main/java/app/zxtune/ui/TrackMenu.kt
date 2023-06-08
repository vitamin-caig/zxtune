package app.zxtune.ui

import android.content.ContentResolver
import android.content.Context
import android.support.v4.media.MediaMetadataCompat
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import androidx.core.view.MenuProvider
import androidx.fragment.app.Fragment
import app.zxtune.MainService
import app.zxtune.R
import app.zxtune.ResultActivity
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
            item(R.id.action_add).enableIf =
                maybeCreateAddCurrentTrackIntent(activity, metadata)
            item(R.id.action_send).enableIf =
                SharingActivity.maybeCreateSendIntent(activity, metadata)
            item(R.id.action_share).enableIf =
                SharingActivity.maybeCreateShareIntent(activity, metadata)
            item(R.id.action_make_ringtone).enableIf =
                metadata.description?.mediaUri?.let { fullId ->
                    RingtoneActivity.createIntent(activity, fullId)
                }
        }
    }

    override fun onMenuItemSelected(menuItem: MenuItem) = when (menuItem.itemId) {
        R.id.action_properties -> {
            model.metadata.value?.let {
                showPropertiesDialog(it)
            }
            true
        }

        else -> false
    }

    private fun showPropertiesDialog(meta: MediaMetadataCompat) =
        InformationFragment.show(activity, meta)
}

private fun maybeCreateAddCurrentTrackIntent(ctx: Context, metadata: MediaMetadataCompat) =
    if (false == isFromProvider(metadata)) {
        ResultActivity.createStartServiceIntent(
            ctx, MainService::class.java, MainService.CUSTOM_ACTION_ADD_CURRENT
        )
    } else {
        null
    }

private fun isFromProvider(metadata: MediaMetadataCompat) =
    metadata.description.mediaId?.startsWith(ContentResolver.SCHEME_CONTENT)
