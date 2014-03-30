/**
 *
 * @file
 *
 * @brief Background playback service
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune;

import java.util.Map;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.os.IBinder;
import android.preference.PreferenceManager;
import android.util.Log;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.PlaybackServiceLocal;
import app.zxtune.rpc.PlaybackServiceServer;
import app.zxtune.ui.StatusNotification;

public class MainService extends Service {

  private final static String TAG = MainService.class.getName();

  private String PREF_MEDIABUTTONS;
  private boolean PREF_MEDIABUTTONS_DEFAULT;
  private String PREF_UNPLUGGING;
  private boolean PREF_UNPLUGGING_DEFAULT;

  private PlaybackServiceLocal service;
  private IBinder binder;
  private Releaseable phoneCallHandler;
  private Releaseable mediaButtonsHandler;
  private Releaseable headphonesPlugHandler;
  private Releaseable settingsChangedHandler;

  @Override
  public void onCreate() {
    Log.d(TAG, "Creating");

    final Resources resources = getResources();
    PREF_MEDIABUTTONS = resources.getString(R.string.pref_control_headset_mediabuttons);
    PREF_MEDIABUTTONS_DEFAULT =
        resources.getBoolean(R.bool.pref_control_headset_mediabuttons_default);
    PREF_UNPLUGGING = resources.getString(R.string.pref_control_headset_unplugging);
    PREF_UNPLUGGING_DEFAULT = resources.getBoolean(R.bool.pref_control_headset_unplugging_default);
    mediaButtonsHandler = ReleaseableStub.instance();
    headphonesPlugHandler = ReleaseableStub.instance();
    
    service = new PlaybackServiceLocal(getApplicationContext());
    final Intent intent = new Intent(this, MainActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    service.subscribe(new StatusNotification(this, intent));
    binder = new PlaybackServiceServer(service);
    final PlaybackControl control = service.getPlaybackControl();
    phoneCallHandler = PhoneCallHandler.subscribe(this, control);
    final SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);
    connectMediaButtons(prefs.getBoolean(PREF_MEDIABUTTONS, PREF_MEDIABUTTONS_DEFAULT));
    connectHeadphonesPlugging(prefs.getBoolean(PREF_UNPLUGGING, PREF_UNPLUGGING_DEFAULT));
    for (Map.Entry<String, ?> entry : prefs.getAll().entrySet()) {
      final String key = entry.getKey();
      if (key.startsWith(ZXTune.Properties.PREFIX)) {
        setProperty(key, entry.getValue(), ZXTune.GlobalOptions.instance());
      }
    }
    settingsChangedHandler =
        new BroadcastReceiverConnection(this, new ChangedSettingsReceiver(), new IntentFilter(
            PreferencesActivity.ACTION_PREFERENCE_CHANGED));
  }

  @Override
  public void onDestroy() {
    Log.d(TAG, "Destroying");
    settingsChangedHandler.release();
    settingsChangedHandler = null;
    headphonesPlugHandler.release();
    headphonesPlugHandler = null;
    mediaButtonsHandler.release();
    mediaButtonsHandler = null;
    phoneCallHandler.release();
    phoneCallHandler = null;
    binder = null;
    service.release();
    service = null;
    stopSelf();
  }

  @Override
  public int onStartCommand(Intent intent, int flags, int startId) {
    Log.d(TAG, "StartCommand called");
    return START_NOT_STICKY;
  }

  @Override
  public IBinder onBind(Intent intent) {
    Log.d(TAG, "onBind called");
    return binder;
  }

  private void connectMediaButtons(boolean connect) {
    Log.d(TAG, "connectMediaButtons = " + connect);
    final boolean connected = mediaButtonsHandler != ReleaseableStub.instance();
    if (connect != connected) {
      mediaButtonsHandler.release();
      mediaButtonsHandler =
          connect
              ? MediaButtonsHandler.subscribe(this, service.getPlaybackControl())
              : ReleaseableStub.instance();
    }
  }

  private void connectHeadphonesPlugging(boolean connect) {
    Log.d(TAG, "connectHeadPhonesPluggins = " + connect);
    final boolean connected = headphonesPlugHandler != ReleaseableStub.instance();
    if (connect != connected) {
      headphonesPlugHandler.release();
      headphonesPlugHandler =
          connect
              ? HeadphonesPlugHandler.subscribe(this, service.getPlaybackControl())
              : ReleaseableStub.instance();
    }
  }

  private class ChangedSettingsReceiver extends BroadcastReceiver {

    @Override
    public void onReceive(Context context, Intent intent) {
      final String key = intent.getStringExtra(PreferencesActivity.EXTRA_PREFERENCE_NAME);
      if (key.equals(PREF_MEDIABUTTONS)) {
        final boolean use =
            intent.getBooleanExtra(PreferencesActivity.EXTRA_PREFERENCE_VALUE,
                PREF_MEDIABUTTONS_DEFAULT);
        connectMediaButtons(use);
      } else if (key.equals(PREF_UNPLUGGING)) {
        final boolean use =
            intent.getBooleanExtra(PreferencesActivity.EXTRA_PREFERENCE_VALUE,
                PREF_UNPLUGGING_DEFAULT);
        connectHeadphonesPlugging(use);
      } else if (key.startsWith(ZXTune.Properties.PREFIX)) {
        final Object value = intent.getExtras().get(PreferencesActivity.EXTRA_PREFERENCE_VALUE);
        setProperty(key, value, ZXTune.GlobalOptions.instance());
      }
    }
  }
  
  private static void setProperty(String name, Object value, ZXTune.Properties.Modifier target) {
    if (value instanceof String) {
      setProperty(name, (String) value, target);
    } else if (value instanceof Long) {
      setProperty(name, ((Long) value).longValue(), target);
    } else if (value instanceof Integer) {
      setProperty(name, ((Integer) value).longValue(), target);
    } else if (value instanceof Boolean) {
      setProperty(name, (Boolean) value ? 1 : 0, target);
    } else {
      throw new RuntimeException("Unknown type of property: " + value.getClass().getName());
    }
  }
  
  private static void setProperty(String name, String value, ZXTune.Properties.Modifier target) {
    try {
      target.setProperty(name, Long.parseLong(value));
    } catch (NumberFormatException e) {
      target.setProperty(name,  value);
    }
  }
  
  private static void setProperty(String name, long value, ZXTune.Properties.Modifier target) {
    target.setProperty(name, value);
  }
}
