/**
 * @file
 * @brief Main application instance
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import android.app.Application;
import app.zxtune.device.ui.Notifications;
import com.github.anrwatchdog.ANRError;
import com.github.anrwatchdog.ANRWatchDog;

public class MainApplication extends Application {

  private static Application instance;

  @Override
  public void onCreate() {
    super.onCreate();

    instance = this;

    Analytics.initialize(this);
    Notifications.setup(this);
    setupANRWatchdog();
  }

  private void setupANRWatchdog() {
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

  public static Application getInstance() {
    return instance;
  }
}
