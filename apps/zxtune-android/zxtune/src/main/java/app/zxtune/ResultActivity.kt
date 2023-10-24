package app.zxtune

import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import app.zxtune.analytics.Analytics
import app.zxtune.device.Permission
import app.zxtune.device.PersistentStorage
import app.zxtune.device.PowerManagement

class ResultActivity : ComponentActivity() {

    companion object {
        private const val ACTION_REQUEST_STORAGE_PERMISSION = "request_storage_permission"
        private const val ACTION_REQUEST_PERSISTENT_STORAGE_LOCATION =
            "request_persistent_storage_location"
        private const val ACTION_SETUP_POWER_MANAGEMENT = "setup_power_management"
        private const val ACTION_REQUEST_PERMISSIONS = "request_permissions"

        // TODO: remove
        private const val ACTION_ADD_TRACKS_TO_PLAYLIST = "add_tracks_to_playlist"

        fun createStoragePermissionRequestIntent(ctx: Context, treeUri: Uri) = Intent(
            ACTION_REQUEST_STORAGE_PERMISSION, treeUri, ctx, ResultActivity::class.java
        )

        fun createPersistentStorageLocationRequestIntent(ctx: Context, treeUri: Uri) = Intent(
            ACTION_REQUEST_PERSISTENT_STORAGE_LOCATION, treeUri, ctx, ResultActivity::class.java
        )

        fun createSetupPowerManagementIntent(ctx: Context) =
            Intent(ctx, ResultActivity::class.java).apply {
                action = ACTION_SETUP_POWER_MANAGEMENT
            }

        fun createPermissionsRequestIntent(ctx: Context, vararg permissions: String) =
            permissions.filter {
                ActivityCompat.checkSelfPermission(ctx, it) != PackageManager.PERMISSION_GRANTED
            }.takeIf { it.isNotEmpty() }?.run {
                Intent(ctx, ResultActivity::class.java).apply {
                    action = ACTION_REQUEST_PERMISSIONS
                    putExtra(
                        ActivityResultContracts.RequestMultiplePermissions.EXTRA_PERMISSIONS,
                        toTypedArray()
                    )
                }
            }

        fun addToPlaylistOrCreateRequestPermissionIntent(ctx: Context, uris: Array<Uri>) =
            if (needRequestNotificationsPermission(ctx)) {
                Intent(ctx, ResultActivity::class.java).apply {
                    action = ACTION_ADD_TRACKS_TO_PLAYLIST
                    putExtra(ScanService.EXTRA_PATHS, uris)
                }
            } else {
                ScanService.add(ctx, uris)
                null
            }

        private fun needRequestNotificationsPermission(ctx: Context) =
            Build.VERSION.SDK_INT >= 33 && ContextCompat.checkSelfPermission(
                ctx, android.Manifest.permission.POST_NOTIFICATIONS
            ) != PackageManager.PERMISSION_GRANTED
    }

    private val storagePermissionRequest =
        registerForActivityResult(ActivityResultContracts.OpenDocumentTree()) { uri: Uri? ->
            if (uri != null && Build.VERSION.SDK_INT >= 19) {
                Permission.requestStorageAccess(this@ResultActivity, uri)
            }
            finish()
        }
    private val persistentStorageLocationRequest =
        registerForActivityResult(ActivityResultContracts.OpenDocumentTree()) { uri: Uri? ->
            uri?.let {
                PersistentStorage.instance.setLocation(it)
            }
            finish()
        }
    private val powerManagement by lazy {
        PowerManagement.create(this)
    }
    private val powerManagementSetupRequest by lazy {
        registerForActivityResult(ActivityResultContracts.StartActivityForResult()) {
            powerManagement.updateState()
            Analytics.sendEvent(
                "power_management",
                "doze" to PowerManagement.dozeEnabled(this),
                "problem" to powerManagement.hasProblem.value
            )
            finish()
        }
    }
    private val permissionRequest =
        registerForActivityResult(ActivityResultContracts.RequestMultiplePermissions()) {
            finish()
        }
    private val notificationPermissionRequest by lazy {
        registerForActivityResult(ActivityResultContracts.RequestPermission()) { isGranted ->
            intent.takeIf { isGranted }?.getParcelableArrayExtra(ScanService.EXTRA_PATHS)
                ?.let { uris ->
                    ScanService.add(
                        applicationContext,
                        Array(uris.size) { idx -> uris[idx] as Uri })
                }
            finish()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val action = intent?.action ?: return
        when (action) {
            ACTION_REQUEST_STORAGE_PERMISSION -> storagePermissionRequest.launch(
                requireNotNull(intent.data)
            )

            ACTION_REQUEST_PERSISTENT_STORAGE_LOCATION -> persistentStorageLocationRequest.launch(
                requireNotNull(intent.data)
            )

            ACTION_SETUP_POWER_MANAGEMENT -> powerManagementSetupRequest.launch(
                powerManagement.createFixitIntent()
            )

            ACTION_REQUEST_PERMISSIONS -> permissionRequest.launch(
                requireNotNull(intent.getStringArrayExtra(ActivityResultContracts.RequestMultiplePermissions.EXTRA_PERMISSIONS))
            )

            ACTION_ADD_TRACKS_TO_PLAYLIST -> if (needRequestNotificationsPermission(this)) {
                notificationPermissionRequest.launch(
                    android.Manifest.permission.POST_NOTIFICATIONS
                )
            }
        }
    }
}
