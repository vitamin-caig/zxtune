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

import android.annotation.TargetApi;
import android.os.Build;
import android.os.DeadObjectException;
import app.zxtune.analytics.Analytics;

import java.util.Arrays;
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
      if (Build.VERSION.SDK_INT >= 24) {
        Analytics.logException(new PrettyException(msg, e));
      } else {
        Analytics.logException(new Exception(msg, e));
      }
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

  @TargetApi(24)
  private static class PrettyException extends Exception {

    PrettyException(String msg, Throwable cause) {
      super(msg, cause, true, true);
      final StackTraceElement[] stack = getStackTrace();
      final String self = Log.class.getName();
      for (int idx = 0, lim = stack.length; idx < lim; ++idx) {
        if (!stack[idx].getClassName().equals(self)) {
          setStackTrace(Arrays.copyOfRange(stack, idx, lim));
          break;
        }
      }
    }
  }
}
