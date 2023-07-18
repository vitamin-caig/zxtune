package app.zxtune

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts
import androidx.activity.result.contract.ActivityResultContracts.OpenDocumentTree
import app.zxtune.analytics.Analytics
import app.zxtune.device.Permission
import app.zxtune.device.PersistentStorage
import app.zxtune.device.PowerManagement

class ResultActivity : ComponentActivity() {

    companion object {
        private const val ACTION_REQUEST_STORAGE_PERMISSION = "request_storage_permission"
        private const val ACTION_REQUEST_PERSISTENT_STORAGE_LOCATION =
            "request_persistent_storage_location"
        private const val ACTION_START_SERVICE = "start_service"
        private const val ACTION_SETUP_POWER_MANAGEMENT = "setup_power_management"
        private const val EXTRA_ORIGINAL_COMPONENT = "extra_original_component"
        private const val EXTRA_ORIGINAL_ACTION = "extra_original_action"

        fun createStoragePermissionRequestIntent(ctx: Context, treeUri: Uri) = Intent(
            ACTION_REQUEST_STORAGE_PERMISSION, treeUri, ctx, ResultActivity::class.java
        )

        fun createPersistentStorageLocationRequestIntent(ctx: Context, treeUri: Uri) = Intent(
            ACTION_REQUEST_PERSISTENT_STORAGE_LOCATION, treeUri, ctx, ResultActivity::class.java
        )

        // Do not store parcelable intent to avoid UnsafeIntentLaunchViolation
        fun createStartServiceIntent(
            ctx: Context, cls: Class<*>, act: String, uri: Uri? = null
        ) = Intent(ctx, ResultActivity::class.java).apply {
            action = ACTION_START_SERVICE
            data = uri
            putExtra(EXTRA_ORIGINAL_COMPONENT, ComponentName(ctx, cls))
            putExtra(EXTRA_ORIGINAL_ACTION, act)
        }

        fun createSetupPowerManagementIntent(ctx: Context) =
            Intent(ctx, ResultActivity::class.java).apply {
                action = ACTION_SETUP_POWER_MANAGEMENT
            }

        private fun getNestedIntent(src: Intent) =
            Intent(requireNotNull(src.getStringExtra(EXTRA_ORIGINAL_ACTION)), src.data).apply {
                component = requireNotNull(src.getParcelableExtra(EXTRA_ORIGINAL_COMPONENT))
            }
    }

    private val storagePermissionRequest =
        registerForActivityResult(OpenDocumentTree()) { uri: Uri? ->
            if (uri != null && Build.VERSION.SDK_INT >= 19) {
                Permission.requestStorageAccess(this@ResultActivity, uri)
            }
            finish()
        }
    private val persistentStorageLocationRequest =
        registerForActivityResult(OpenDocumentTree()) { uri: Uri? ->
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
            Analytics.sendEvent("power_management",
                "doze" to PowerManagement.dozeEnabled(this),
                "problem" to powerManagement.hasProblem.value)
            finish()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val action = intent?.action ?: return
        when (action) {
            ACTION_START_SERVICE -> {
                startService(getNestedIntent(intent))
                finish()
            }

            ACTION_REQUEST_STORAGE_PERMISSION -> storagePermissionRequest.launch(
                requireNotNull(intent.data)
            )

            ACTION_REQUEST_PERSISTENT_STORAGE_LOCATION -> persistentStorageLocationRequest.launch(
                requireNotNull(intent.data)
            )

            ACTION_SETUP_POWER_MANAGEMENT -> powerManagementSetupRequest.launch(
                powerManagement.createFixitIntent()
            )
        }
    }
}
