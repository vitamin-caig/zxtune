/**
 * @file
 * @brief Permission request helper
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.device

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.os.Build
import android.provider.Settings
import androidx.annotation.RequiresApi

object Permission {
    fun canWriteSystemSettings(ctx: Context) =
        Build.VERSION.SDK_INT < 23 || Settings.System.canWrite(ctx)

    @RequiresApi(19)
    fun requestStorageAccess(ctx: Context, uri: Uri) =
        ctx.contentResolver.takePersistableUriPermission(
            uri, Intent.FLAG_GRANT_READ_URI_PERMISSION
        )

    @RequiresApi(19)
    fun requestPersistentStorageAccess(ctx: Context, uri: Uri) =
        ctx.contentResolver.takePersistableUriPermission(
            uri, Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION
        )
}
