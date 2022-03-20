/**
 * @file
 * @brief Main application instance
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import android.app.Application;
import android.content.Context;
import android.os.Build;

import androidx.annotation.Nullable;

import com.github.anrwatchdog.ANRWatchDog;

import app.zxtune.analytics.Analytics;
import app.zxtune.device.ui.Notifications;
import app.zxtune.net.NetworkManager;

public class MainApplication extends Application {

  private static final Analytics.Trace TRACE = Analytics.Trace.create("MainApplication");

  @Nullable
  private static Context globalContext;

  @Override
  public void onCreate() {
    TRACE.beginMethod("onCreate");
    TRACE.checkpoint("step");

    super.onCreate();
    TRACE.checkpoint("super");

    initialize(this);
    TRACE.endMethod();
  }

  public synchronized static void initialize(Context ctx) {
    if (globalContext == null) {
      globalContext = ctx;
      if (!"robolectric".equals(Build.PRODUCT)) {
        NetworkManager.initialize(ctx);
        Analytics.initialize(ctx);
        Notifications.setup(ctx);
        setupANRWatchdog();
      }
    }
  }

  private static void setupANRWatchdog() {
    // Report only main thread due to way too big report - https://github.com/SalomonBrys/ANR-WatchDog/issues/29
    new ANRWatchDog(2000)
        .setReportMainThreadOnly()
        .setANRListener(error -> Log.w("ANR", error, "Hangup")).start();
  }

  public synchronized static Context getGlobalContext() {
    return globalContext;
  }
}
