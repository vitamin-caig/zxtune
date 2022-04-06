package app.zxtune

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.result.contract.ActivityResultContracts.OpenDocumentTree
import app.zxtune.device.Permission
import app.zxtune.device.PersistentStorage
import app.zxtune.utils.ifNotNulls

class ResultActivity : ComponentActivity() {

    companion object {
        private const val ACTION_REQUEST_STORAGE_PERMISSION = "request_storage_permission"
        private const val ACTION_REQUEST_PERSISTENT_STORAGE_LOCATION =
            "request_persistent_storage_location"

        fun createStoragePermissionRequestIntent(ctx: Context, treeUri: Uri) = Intent(
            ACTION_REQUEST_STORAGE_PERMISSION,
            treeUri,
            ctx,
            ResultActivity::class.java
        )

        fun createPersistentStorageLocationRequestIntent(ctx: Context, treeUri: Uri) = Intent(
            ACTION_REQUEST_PERSISTENT_STORAGE_LOCATION, treeUri, ctx,
            ResultActivity::class.java
        )
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

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        ifNotNulls(intent?.action, intent?.data, this::processIntent)
    }

    private fun processIntent(action: String, data: Uri) = when (action) {
        ACTION_REQUEST_STORAGE_PERMISSION -> storagePermissionRequest.launch(data)
        ACTION_REQUEST_PERSISTENT_STORAGE_LOCATION -> persistentStorageLocationRequest.launch(data)
        else -> Unit
    }
}
