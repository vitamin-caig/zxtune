package app.zxtune.fs.provider

import android.content.Context
import android.content.Intent
import android.database.MatrixCursor
import android.net.Uri
import android.provider.Settings
import androidx.annotation.VisibleForTesting
import androidx.lifecycle.LiveData
import androidx.lifecycle.Observer
import app.zxtune.Features
import app.zxtune.R
import app.zxtune.device.PersistentStorage
import app.zxtune.fs.VfsObject
import app.zxtune.fs.permissionQueryIntent
import app.zxtune.net.NetworkManager

internal class NotificationsSource @VisibleForTesting constructor(
    private val ctx: Context,
    private val networkState: LiveData<Boolean>,
    private val persistentStorageSetupIntent: LiveData<Intent?>,
) {
    private val statesObserver = Observer<Any?> {
        ctx.contentResolver?.notifyChange(ROOT_NOTIFICATION_URI, null)
    }

    init {
        networkState.observeForever(statesObserver)
        persistentStorageSetupIntent.observeForever(statesObserver)
    }

    constructor(ctx: Context) : this(
        ctx, NetworkManager.from(ctx).networkAvailable, PersistentStorage.instance.setupIntent
    )

    fun shutdown() {
        networkState.removeObserver(statesObserver)
        persistentStorageSetupIntent.removeObserver(statesObserver)
    }

    fun getFor(obj: VfsObject) = getNotification(obj)?.let { notification ->
        MatrixCursor(Schema.Notifications.COLUMNS).apply {
            addRow(notification.serialize())
        }
    }

    @VisibleForTesting
    fun getNotification(obj: VfsObject): Schema.Notifications.Object? {
        val uri = obj.uri
        return when (uri.scheme) {
            null -> null // root
            "file" -> getStorageNotification(obj)
            "playlists" -> getPlaylistNotification(uri)
            else -> getNetworkNotification()
        }
    }

    private fun getNetworkNotification(): Schema.Notifications.Object? {
        return if (false == networkState.value) {
            Schema.Notifications.Object(
                ctx.getString(R.string.network_inaccessible),
                Intent(Settings.ACTION_WIRELESS_SETTINGS)
            )
        } else {
            null
        }
    }

    private fun getStorageNotification(obj: VfsObject) = obj.permissionQueryIntent?.let {
        Schema.Notifications.Object(
            ctx.getString(R.string.limited_storage_access), it
        )
    }

    private fun getPlaylistNotification(uri: Uri): Schema.Notifications.Object? {
        // show only for root
        if (!uri.path.isNullOrEmpty() || !Features.StorageAccessFramework.isEnabled()) {
            return null
        }
        return persistentStorageSetupIntent.value?.let {
            Schema.Notifications.Object(ctx.getString(R.string.no_stored_playlists_access), it)
        }
    }

    companion object {
        private val ROOT_NOTIFICATION_URI = Query.notificationUriFor(Uri.EMPTY)
    }
}
