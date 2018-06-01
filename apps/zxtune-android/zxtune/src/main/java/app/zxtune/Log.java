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
    if (BuildConfig.DEBUG) {
      android.util.Log.d(tag, msg);
    }
  }

  public static void d(String tag, String msg, Object... params) {
    if (BuildConfig.DEBUG) {
      android.util.Log.d(tag, String.format(Locale.US, msg, params));
    }
  }

  public static void w(String tag, Throwable e, String msg) {
    if (BuildConfig.DEBUG) {
      android.util.Log.w(tag, msg, e);
    }
    if (needLogException(e)) {
      Analytics.logException(e);
    }
  }

  public static void w(String tag, Throwable e, String msg, Object... params) {
    w(tag, e, String.format(Locale.US, msg, params));
  }

  private static boolean needLogException(Throwable e) {
    if (e instanceof DeadObjectException) {
      if (deadObjectExceptionLogged) {
        return false;
      }
      deadObjectExceptionLogged = true;
    }
    return true;
  }
}
