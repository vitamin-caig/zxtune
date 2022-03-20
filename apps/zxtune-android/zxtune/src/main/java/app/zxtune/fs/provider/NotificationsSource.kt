package app.zxtune.fs.provider

import android.content.Context
import android.content.Intent
import android.database.MatrixCursor
import android.net.Uri
import android.os.Build
import android.os.Environment
import android.provider.Settings
import androidx.annotation.VisibleForTesting
import androidx.lifecycle.LiveData
import androidx.lifecycle.Observer
import app.zxtune.R
import app.zxtune.net.NetworkManager
import java.io.File

internal class NotificationsSource @VisibleForTesting constructor(
    private val ctx: Context,
    private val networkState: LiveData<Boolean>,
) {
    private val networkStateObserver = Observer<Boolean> {
        resolverNotificationUri?.let { uri ->
            ctx.contentResolver?.notifyChange(uri, null)
        }
    }
    private var resolverNotificationUri: Uri? = null

    init {
        networkState.observeForever(networkStateObserver)
    }

    constructor(ctx: Context) : this(ctx, getNetworkState(ctx))

    fun getFor(uri: Uri) = getNotification(uri)?.let { obj ->
        MatrixCursor(Schema.Notifications.COLUMNS).apply {
            addRow(obj.serialize())
        }
    }

    @VisibleForTesting
    fun getNotification(uri: Uri): Schema.Notifications.Object? {
        resolverNotificationUri = null
        return when (uri.scheme) {
            null -> null // root
            "file" -> getStorageNotification(uri)
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

    private fun getStorageNotification(uri: Uri) =
        if (Build.VERSION.SDK_INT >= 30 &&
            true == uri.path?.takeIf { it.isNotEmpty() }?.let {
                !Environment.isExternalStorageManager(File(it))
            }
        ) {
            Schema.Notifications.Object(
                ctx.getString(R.string.limited_storage_access),
                Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
            )
        } else {
            null
        }

    private fun getPlaylistNotification(uri: Uri) =
        if (Build.VERSION.SDK_INT >= 30 && uri.path.isNullOrEmpty() && !Environment.isExternalStorageManager()) {
            Schema.Notifications.Object(
                ctx.getString(R.string.limited_stored_playlists_access),
                Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION)
            )
        } else {
            null
        }

    companion object {
        private fun getNetworkState(ctx: Context): LiveData<Boolean> {
            NetworkManager.initialize(ctx.applicationContext)
            return NetworkManager.networkAvailable
        }
    }
}
