/**
 * 
 * @file
 *
 * @brief Suitable replacement for android.util.Log
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune;

import java.util.Locale;

public final class Log {
  
  // public interface
  public static void d(String tag, String msg) {
    android.util.Log.d(tag, msg);
  }

  public static void d(String tag, String msg, Object... params) {
    d(tag, String.format(Locale.US, msg, params));
  }

  public static void d(String tag, Throwable e, String msg) {
    android.util.Log.d(tag, msg, e);
  }

  public static void d(String tag, Throwable e, String msg, Object... params) {
    d(tag, e, String.format(Locale.US, msg, params));
  }
}
