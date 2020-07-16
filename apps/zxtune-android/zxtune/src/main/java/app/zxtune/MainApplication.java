/**
 * @file
 * @brief Main application instance
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import android.app.Application;
import android.content.Context;

import androidx.annotation.Nullable;

import app.zxtune.analytics.Analytics;
import app.zxtune.device.ui.Notifications;
import com.github.anrwatchdog.ANRError;
import com.github.anrwatchdog.ANRWatchDog;

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
      Analytics.initialize(ctx);
      Notifications.setup(ctx);
      setupANRWatchdog();
    }
  }

  private static void setupANRWatchdog() {
    // Report only main thread due to way too big report - https://github.com/SalomonBrys/ANR-WatchDog/issues/29
    new ANRWatchDog(2000)
        .setReportMainThreadOnly()
        .setANRListener(new ANRWatchDog.ANRListener() {
      @Override
      public void onAppNotResponding(ANRError error) {
        Log.w("ANR", error, "Hangup");
      }
    }).start();
  }

  public synchronized static Context getGlobalContext() {
    return globalContext;
  }
}
