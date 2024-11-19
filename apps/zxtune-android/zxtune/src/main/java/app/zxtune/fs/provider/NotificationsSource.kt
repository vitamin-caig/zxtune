package app.zxtune.fs.provider

import android.content.Context
import android.content.Intent
import android.database.MatrixCursor
import android.net.Uri
import android.provider.Settings
import androidx.annotation.VisibleForTesting
import app.zxtune.Features
import app.zxtune.R
import app.zxtune.device.PersistentStorage
import app.zxtune.fs.VfsObject
import app.zxtune.fs.permissionQueryIntent
import app.zxtune.net.NetworkManager
import app.zxtune.ui.utils.flowValueOf
import kotlinx.coroutines.CoroutineDispatcher
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.FlowPreview
import kotlinx.coroutines.cancel
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.debounce
import kotlinx.coroutines.flow.merge
import kotlinx.coroutines.launch

internal class NotificationsSource @VisibleForTesting constructor(
    private val ctx: Context,
    private val networkState: StateFlow<Boolean>,
    persistentStorageSetupIntentFlow: Flow<Intent?>,
    dispatcher: CoroutineDispatcher,
) {
    private val scope = CoroutineScope(dispatcher)
    private val persistentStorageSetupIntent by flowValueOf(
        persistentStorageSetupIntentFlow, scope, null
    )

    init {
        @OptIn(FlowPreview::class) scope.launch {
            merge(networkState, persistentStorageSetupIntentFlow).debounce(1000).collect {
                ctx.contentResolver?.notifyChange(ROOT_NOTIFICATION_URI, null)
            }
        }
    }

    constructor(ctx: Context) : this(
        ctx,
        NetworkManager.from(ctx).networkAvailable,
        PersistentStorage.instance.setupIntent,
        Dispatchers.Main,
    )

    fun shutdown() = scope.cancel()

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

    private fun getNetworkNotification() = if (!networkState.value) {
        Schema.Notifications.Object(
            ctx.getString(R.string.network_inaccessible), Intent(Settings.ACTION_WIRELESS_SETTINGS)
        )
    } else {
        null
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
        return persistentStorageSetupIntent?.let {
            Schema.Notifications.Object(ctx.getString(R.string.no_stored_playlists_access), it)
        }
    }

    companion object {
        private val ROOT_NOTIFICATION_URI = Query.notificationUriFor(Uri.EMPTY)
    }
}
