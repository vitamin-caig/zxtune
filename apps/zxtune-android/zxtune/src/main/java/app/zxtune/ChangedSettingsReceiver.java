package app.zxtune;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;

import java.util.Map;

import app.zxtune.core.Properties;
import app.zxtune.core.PropertiesModifier;
import app.zxtune.core.jni.GlobalOptions;

class ChangedSettingsReceiver extends BroadcastReceiver {

  private static final String TAG = ChangedSettingsReceiver.class.getName();

  private ChangedSettingsReceiver() {
  }

  static Releaseable subscribe(Context ctx) {
    final SharedPreferences prefs = Preferences.getDefaultSharedPreferences(ctx);
    for (Map.Entry<String, ?> entry : prefs.getAll().entrySet()) {
      final String key = entry.getKey();
      if (key.startsWith(Properties.PREFIX)) {
        setProperty(key, entry.getValue(), GlobalOptions.instance());
      }
    }
    return new BroadcastReceiverConnection(ctx,
        new ChangedSettingsReceiver(),
        new IntentFilter(PreferencesActivity.ACTION_PREFERENCE_CHANGED)
    );
  }

  @Override
  public void onReceive(Context context, Intent intent) {
    final String key = intent.getStringExtra(PreferencesActivity.EXTRA_PREFERENCE_NAME);
    if (key.startsWith(Properties.PREFIX)) {
      final Object value = intent.getExtras().get(PreferencesActivity.EXTRA_PREFERENCE_VALUE);
      if (value != null) {
        setProperty(key, value, GlobalOptions.instance());
      }
    }
  }

  private static void setProperty(String name, Object value, PropertiesModifier target) {
    try {
      if (value instanceof String) {
        setProperty(name, (String) value, target);
      } else if (value instanceof Long) {
        setProperty(name, ((Long) value).longValue(), target);
      } else if (value instanceof Integer) {
        setProperty(name, ((Integer) value).longValue(), target);
      } else if (value instanceof Boolean) {
        setProperty(name, (Boolean) value ? 1 : 0, target);
      } else {
        throw new Exception("Unknown type of property: " + value.getClass().getName());
      }
    } catch (Exception e) {
      Log.w(TAG, e, "setProperty");
    }
  }

  private static void setProperty(String name, String value, PropertiesModifier target) throws Exception {
    try {
      target.setProperty(name, Long.parseLong(value));
    } catch (NumberFormatException e) {
      target.setProperty(name, value);
    }
  }

  private static void setProperty(String name, long value, PropertiesModifier target) throws Exception {
    target.setProperty(name, value);
  }
}