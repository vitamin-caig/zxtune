/**
 *
 * @file
 *
 * @brief Permission request helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.os.Build;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;

public class Permission {
  
  public static void request(Activity act, String id) {
    if (Build.VERSION.SDK_INT >= 23) {
      requestMarshmallow(act, id);
    }
  }
  
  @TargetApi(23)
  private static void requestMarshmallow(Activity act, String id) {
    if (ContextCompat.checkSelfPermission(act, id) != PackageManager.PERMISSION_GRANTED) {
      ActivityCompat.requestPermissions(act, new String[]{id}, id.hashCode() & 0xffff);
    }
  }
}
