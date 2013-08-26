/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.PreferenceActivity;

public class PreferencesActivity extends PreferenceActivity implements OnSharedPreferenceChangeListener {

  public final static String ACTION_PREFERENCE_CHANGED = PreferencesActivity.class.getName() + ".PREFERENCE_CHANGED";
  public final static String EXTRA_PREFERENCE_NAME = "name";
  public final static String EXTRA_PREFERENCE_VALUE = "value";
    
  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    addPreferencesFromResource(R.xml.preferences);
  }
  
  @Override
  protected void onResume() {
    super.onResume();
    getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener(this);
  }
  
  @Override
  protected void onPause() {
    super.onPause();
    getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener(this);
  }

  @Override
  public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
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
}
