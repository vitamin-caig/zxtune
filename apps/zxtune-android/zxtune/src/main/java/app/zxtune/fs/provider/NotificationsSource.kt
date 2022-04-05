package app.zxtune.fs.provider

import android.content.Context
import android.content.Intent
import android.database.MatrixCursor
import android.net.Uri
import android.os.Build
import android.provider.Settings
import androidx.annotation.VisibleForTesting
import androidx.lifecycle.LiveData
import androidx.lifecycle.Observer
import app.zxtune.R
import app.zxtune.ResultActivity
import app.zxtune.device.PersistentStorage
import app.zxtune.fs.VfsObject
import app.zxtune.fs.VfsRootLocalStorageAccessFramework
import app.zxtune.fs.permissionQueryUri
import app.zxtune.net.NetworkManager

internal class NotificationsSource @VisibleForTesting constructor(
    private val ctx: Context,
    private val networkState: LiveData<Boolean>,
    private val persistentStorageState: LiveData<PersistentStorage.State>,
) {
    private val statesObserver = Observer<Any> {
        resolverNotificationUri?.let { uri ->
            ctx.contentResolver?.notifyChange(uri, null)
        }
    }
    private var resolverNotificationUri: Uri? = null

    init {
        networkState.observeForever(statesObserver)
        persistentStorageState.observeForever(statesObserver)
    }

    constructor(ctx: Context) : this(ctx, getNetworkState(ctx), PersistentStorage.instance.state)

    fun getFor(obj: VfsObject) = getNotification(obj)?.let { notification ->
        MatrixCursor(Schema.Notifications.COLUMNS).apply {
            addRow(notification.serialize())
        }
    }

    @VisibleForTesting
    fun getNotification(obj: VfsObject): Schema.Notifications.Object? {
        resolverNotificationUri = null
        val uri = obj.uri
        return when (uri.scheme) {
            null -> null // root
            "file" -> getStorageNotification(obj)
            "playlists" -> getPlaylistNotification(uri)
            else -> getNetworkNotification(uri)
        }
    }

    private fun getNetworkNotification(uri: Uri): Schema.Notifications.Object? {
        resolverNotificationUri = Query.notificationUriFor(uri)
        return if (false == networkState.value) {
            Schema.Notifications.Object(
                ctx.getString(R.string.network_inaccessible),
                Intent(Settings.ACTION_WIRELESS_SETTINGS)
            )
        } else {
            null
        }
    }

    private fun getStorageNotification(obj: VfsObject) = obj.permissionQueryUri?.let { uri ->
        Schema.Notifications.Object(
            ctx.getString(R.string.limited_storage_access),
            ResultActivity.createStoragePermissionRequestIntent(ctx, uri)
        )
    }

    private fun getPlaylistNotification(uri: Uri): Schema.Notifications.Object? {
        // show only for root
        if (!uri.path.isNullOrEmpty()) {
            return null
        }
        if (Build.VERSION.SDK_INT < VfsRootLocalStorageAccessFramework.REQUIRED_SDK_LEVEL) {
            return null
        }
        resolverNotificationUri = Query.notificationUriFor(uri)
        persistentStorageState.value?.let { state ->
            if (true != state.location?.isDirectory) {
                return Schema.Notifications.Object(
                    ctx.getString(R.string.no_stored_playlists_access),
                    ResultActivity.createPersistentStorageLocationRequestIntent(
                        ctx,
                        state.defaultLocationHint
                    )
                )
            }
        }
        return null
    }

    companion object {
        private fun getNetworkState(ctx: Context): LiveData<Boolean> {
            NetworkManager.initialize(ctx.applicationContext)
            return NetworkManager.networkAvailable
        }
    }
}
