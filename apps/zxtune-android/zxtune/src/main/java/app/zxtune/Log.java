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

import android.os.DeadObjectException;

import java.util.Locale;

public final class Log {

  private static boolean deadObjectExceptionLogged = false;
  
  // public interface
  public static void d(String tag, String msg) {
    android.util.Log.d(tag, msg);
  }

  public static void d(String tag, String msg, Object... params) {
    d(tag, String.format(Locale.US, msg, params));
  }

  public static void w(String tag, Throwable e, String msg) {
    android.util.Log.w(tag, msg, e);
    if (e instanceof DeadObjectException) {
      logDeadObjectException(e);
    } else {
      Analytics.logException(e);
    }
  }

  public static void w(String tag, Throwable e, String msg, Object... params) {
    w(tag, e, String.format(Locale.US, msg, params));
  }

  private static void logDeadObjectException(Throwable e) {
    if (!deadObjectExceptionLogged) {
      deadObjectExceptionLogged = true;
      Analytics.logException(e);
    }
  }
}
