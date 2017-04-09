/**
 *
 * @file
 *
 * @brief Preferences activity
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.CheckBoxPreference;
import android.preference.ListPreference;
import android.preference.Preference;
import android.preference.PreferenceActivity;
import android.preference.PreferenceGroup;
import android.preference.PreferenceScreen;

import app.zxtune.ui.StatusNotification;

public class PreferencesActivity extends PreferenceActivity implements OnSharedPreferenceChangeListener {

  public static final String ACTION_PREFERENCE_CHANGED = PreferencesActivity.class.getName() + ".PREFERENCE_CHANGED";
  public static final String EXTRA_PREFERENCE_NAME = "name";
  public static final String EXTRA_PREFERENCE_VALUE = "value";
    
  @SuppressWarnings("deprecation")
  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.preferences_ui);
    addPreferencesFromResource(R.xml.preferences_emulation);
    addPreferencesFromResource(R.xml.preferences_sound);
    hideUnsupportedPreferences(getPreferenceScreen());
    initPreferenceSummary(getPreferenceScreen());
  }
  
  @SuppressWarnings("deprecation")
  @Override
  protected void onResume() {
    super.onResume();
    getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
  }
  
  @SuppressWarnings("deprecation")
  @Override
  protected void onPause() {
    super.onPause();
    getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
  }

  @SuppressWarnings("deprecation")
  @Override
  public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
    updatePreferenceSummary(findPreference(key));
    final Intent intent = new Intent(ACTION_PREFERENCE_CHANGED);
    intent.putExtra(EXTRA_PREFERENCE_NAME, key);
    final Object val = getPreferenceScreen().getSharedPreferences().getAll().get(key);
    if (val instanceof String) {
      intent.putExtra(EXTRA_PREFERENCE_VALUE, (String) val);
    } else if (val instanceof Integer) {
      intent.putExtra(EXTRA_PREFERENCE_VALUE, (Integer) val);
    } else if (val instanceof Long) {
      intent.putExtra(EXTRA_PREFERENCE_VALUE, (Long) val);
    } else if (val instanceof Boolean) {
      intent.putExtra(EXTRA_PREFERENCE_VALUE, (Boolean) val);
    }
    sendBroadcast(intent);
  }
  
  private void hideUnsupportedPreferences(PreferenceScreen root) {
    if (!StatusNotification.BUTTONS_SUPPORTED) {
      disablePreference(root, R.string.pref_control_notification_buttons);
    }
  }
  
  private void disablePreference(PreferenceScreen root, int keyId) {
    final String key = getResources().getString(keyId);
    final Preference pref = root.findPreference(key);
    pref.setEnabled(false);
    if (pref instanceof CheckBoxPreference) {
      ((CheckBoxPreference)pref).setChecked(false);
    }
  }

  private static void initPreferenceSummary(PreferenceGroup group) {
    for (int idx = group.getPreferenceCount(); idx > 0; --idx) {
      initPreferenceSummary(group.getPreference(idx - 1));
    }
  }
  
  private static void initPreferenceSummary(Preference pref) {
    if (pref instanceof PreferenceGroup) {
      initPreferenceSummary((PreferenceGroup) pref);
    } else {
      updatePreferenceSummary(pref);
    }
  }
  
  private static void updatePreferenceSummary(Preference pref) {
    if (pref instanceof ListPreference) {
      final ListPreference list = (ListPreference) pref;
      list.setSummary(list.getEntry());
    }
  }
}
