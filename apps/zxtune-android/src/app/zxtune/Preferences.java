/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune;

import android.annotation.TargetApi;
import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.preference.PreferenceManager;

public class Preferences {

  private Preferences() {
  }

  public static SharedPreferences getDefaultSharedPreferences(Context context) {
    return Build.VERSION.SDK_INT < Build.VERSION_CODES.HONEYCOMB
      ? PreferenceManager.getDefaultSharedPreferences(context)
      : context.getSharedPreferences(getDefaultSharedPreferencesName(context),
          getDefaultSharedPreferencesMode());
  }

  private static String getDefaultSharedPreferencesName(Context context) {
    return context.getPackageName() + "_preferences";
  }
  
  @TargetApi(Build.VERSION_CODES.HONEYCOMB)
  private static int getDefaultSharedPreferencesMode() {
    return Context.MODE_PRIVATE | Context.MODE_MULTI_PROCESS;
  }
}
