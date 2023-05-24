package app.zxtune.ui

import android.content.ContentResolver
import android.content.Context
import android.content.Intent
import android.support.v4.media.MediaMetadataCompat
import android.view.Menu
import android.view.MenuInflater
import android.view.MenuItem
import androidx.annotation.IdRes
import androidx.core.view.MenuProvider
import androidx.fragment.app.Fragment
import app.zxtune.MainService
import app.zxtune.R
import app.zxtune.ResultActivity
import app.zxtune.SharingActivity
import app.zxtune.device.media.MediaModel

class TrackMenu(private val fragment: Fragment) : MenuProvider {
    private val activity
        get() = fragment.requireActivity()
    private val model
        get() = MediaModel.of(activity)

    override fun onCreateMenu(menu: Menu, menuInflater: MenuInflater) =
        menuInflater.inflate(R.menu.track, menu)

    override fun onPrepareMenu(menu: Menu) = menu.run {
        model.metadata.observe(fragment.viewLifecycleOwner) { metadata ->
            item(R.id.action_track).isEnabled = metadata != null
            if (metadata == null) {
                return@observe
            }
            item(R.id.action_add).action = maybeCreateAddCurrentTrackIntent(activity, metadata)
            item(R.id.action_send).action =
                SharingActivity.maybeCreateSendIntent(activity, metadata)
            item(R.id.action_share).action =
                SharingActivity.maybeCreateShareIntent(activity, metadata)
            item(R.id.action_make_ringtone).action =
                metadata.description?.mediaUri?.let { fullId ->
                    RingtoneActivity.createIntent(activity, fullId)
                }
        }
    }

    override fun onMenuItemSelected(menuItem: MenuItem) = false
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

private var MenuItem.action: Intent?
    get() = intent
    set(data) {
        isEnabled = data != null
        intent = data
    }

private fun Menu.item(@IdRes id: Int) = requireNotNull(findItem(id))
