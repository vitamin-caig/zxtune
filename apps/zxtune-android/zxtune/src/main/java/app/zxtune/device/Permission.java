/**
 * @file
 * @brief Permission request helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.device;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;

import androidx.annotation.RequiresApi;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.fragment.app.FragmentActivity;

public class Permission {

  public static void request(FragmentActivity act, String id) {
    if (Build.VERSION.SDK_INT >= 23) {
      requestMarshmallow(act, id);
    }
  }

  public static boolean requestSystemSettings(FragmentActivity act) {
    if (Build.VERSION.SDK_INT >= 23) {
      return requestSystemSettingsMarshmallow(act);
    } else {
      return true;
    }
  }

  @RequiresApi(19)
  public static void requestStorageAccess(Context ctx, Uri uri) {
    ctx.getContentResolver().takePersistableUriPermission(uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
  }

  @TargetApi(23)
  private static void requestMarshmallow(FragmentActivity act, String id) {
    if (ContextCompat.checkSelfPermission(act, id) != PackageManager.PERMISSION_GRANTED) {
      ActivityCompat.requestPermissions(act, new String[]{id}, id.hashCode() & 0xffff);
    }
  }

  @TargetApi(23)
  private static boolean requestSystemSettingsMarshmallow(Context ctx) {
    if (Settings.System.canWrite(ctx)) {
      return true;
    }
    final Intent intent = new Intent(Settings.ACTION_MANAGE_WRITE_SETTINGS);
    intent.setData(Uri.parse("package:" + ctx.getPackageName()));
    ctx.startActivity(intent);
    return false;
  }
}
