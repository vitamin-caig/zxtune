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

import android.content.Context;
import android.content.SharedPreferences;

public class Preferences {

  private Preferences() {
  }

  public static SharedPreferences getDefaultSharedPreferences(Context context) {
    return context.getSharedPreferences(getDefaultSharedPreferencesName(context),
          getDefaultSharedPreferencesMode());
  }

  private static String getDefaultSharedPreferencesName(Context context) {
    return context.getPackageName() + "_preferences";
  }
  
  private static int getDefaultSharedPreferencesMode() {
    return Context.MODE_PRIVATE | Context.MODE_MULTI_PROCESS;
  }
}
