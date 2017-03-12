/**
 * 
 * @file
 *
 * @brief Main application instance
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune;

import android.app.Application;

public class MainApplication extends Application {
  
  private static Application instance;

  @Override
  public void onCreate() {
    super.onCreate();

    instance = this;

    Analytics.initialize(this);
  }
  
  public static Application getInstance() {
    return instance;
  }
}
