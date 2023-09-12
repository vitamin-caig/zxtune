/**
 * @file
 * @brief Permission request helper
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.device

import android.annotation.TargetApi
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.provider.Settings
import androidx.annotation.RequiresApi
import androidx.core.app.ActivityCompat
import androidx.fragment.app.FragmentActivity

object Permission {
    @JvmStatic
    fun request(act: FragmentActivity, vararg ids: String) {
        if (Build.VERSION.SDK_INT >= 23) {
            requestMarshmallow(act, *ids)
        }
    }

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

    @TargetApi(23)
    private fun requestMarshmallow(act: FragmentActivity, vararg ids: String) = ids.filter {
        ActivityCompat.checkSelfPermission(act, it) != PackageManager.PERMISSION_GRANTED
    }.takeIf { it.isNotEmpty() }?.run {
        ActivityCompat.requestPermissions(act, toTypedArray(), hashCode() and 0xffff)
    } ?: Unit
}
