package app.zxtune.preferences;

import android.content.SharedPreferences;

import java.util.Map;

import app.zxtune.Releaseable;
import app.zxtune.core.PropertiesModifier;

/**
 * Gate between SharedPreferences and PropertiesModifier
 */
public class SharedPreferencesBridge {

  public static Releaseable subscribe(SharedPreferences preferences, PropertiesModifier target) {
    final RawPropertiesAdapter adapter = new RawPropertiesAdapter(target);
    dumpExistingProperties(preferences.getAll(), adapter);
    SharedPreferences.OnSharedPreferenceChangeListener listener =
        (SharedPreferences updated, String key) -> {
          final Object value = updated.getAll().get(key);
          adapter.setProperty(key, value);
        };
    preferences.registerOnSharedPreferenceChangeListener(listener);
    return () -> preferences.unregisterOnSharedPreferenceChangeListener(listener);
  }

  private static void dumpExistingProperties(Map<String, ?> prefs, RawPropertiesAdapter target) {
    for (Map.Entry<String, ?> entry : prefs.entrySet()) {
      target.setProperty(entry.getKey(), entry.getValue());
    }
  }
}
