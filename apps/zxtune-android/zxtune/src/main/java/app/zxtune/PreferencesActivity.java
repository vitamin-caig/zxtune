/**
 * @file
 * @brief Preferences activity
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.annotation.XmlRes;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.FragmentManager;
import androidx.preference.Preference;
import androidx.preference.PreferenceFragmentCompat;

public class PreferencesActivity extends AppCompatActivity implements PreferenceFragmentCompat.OnPreferenceStartFragmentCallback {

  public static final String ACTION_PREFERENCE_CHANGED = PreferencesActivity.class.getName() + ".PREFERENCE_CHANGED";
  public static final String EXTRA_PREFERENCE_NAME = "name";
  public static final String EXTRA_PREFERENCE_VALUE = "value";

  private Releaseable broadcast = ReleaseableStub.instance();

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    // Need to only create the first fragment
    FragmentManager mgr = getSupportFragmentManager();
    if (mgr.findFragmentById(android.R.id.content) == null) {
      mgr.beginTransaction()
          .add(android.R.id.content, new MainFragment())
          .disallowAddToBackStack()
          .commit();
    }
  }

  @Override
  public void onResume() {
    super.onResume();

    broadcast = SharedPreferenceBroadcast.subscribe(this, Preferences.getDefaultSharedPreferences(this));
  }

  @Override
  public void onPause() {
    super.onPause();

    broadcast.release();
    broadcast = ReleaseableStub.instance();
  }

  @Override
  public boolean onPreferenceStartFragment(PreferenceFragmentCompat caller, Preference pref) {
    if ("zxtune.sound.mixer.3".equals(pref.getKey())) {
      getSupportFragmentManager()
          .beginTransaction()
          .replace(android.R.id.content, new Mixer3Fragment())
          .addToBackStack(null)
          .commit();
      return true;
    }
    return false;
  }

  public static class MainFragment extends Fragment {
    public MainFragment() {
      super(R.xml.preferences);
    }
  }

  public static class Mixer3Fragment extends Fragment {
    public Mixer3Fragment() {
      super(R.xml.preferences_mixer3);
    }
  }

  public static class Fragment extends PreferenceFragmentCompat {

    @XmlRes
    private final int layout;

    protected Fragment(@XmlRes int layout) {
      this.layout = layout;
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
      setPreferencesFromResource(layout, rootKey);
    }
  }

  private static class SharedPreferenceBroadcast implements SharedPreferences.OnSharedPreferenceChangeListener {

    private final SharedPreferences preferences;
    private final Context context;

    static Releaseable subscribe(Context ctx, SharedPreferences prefs) {
      SharedPreferenceBroadcast self = new SharedPreferenceBroadcast(prefs, ctx);
      prefs.registerOnSharedPreferenceChangeListener(self);
      return () -> {
        prefs.unregisterOnSharedPreferenceChangeListener(self);
      };
    }

    private SharedPreferenceBroadcast(SharedPreferences prefs, Context ctx) {
      preferences = prefs;
      context = ctx;
    }

    @Override
    public void onSharedPreferenceChanged(SharedPreferences sharedPreferences, String key) {
      final Intent intent = new Intent(ACTION_PREFERENCE_CHANGED);
      intent.putExtra(EXTRA_PREFERENCE_NAME, key);
      final Object val = preferences.getAll().get(key);
      if (val instanceof String) {
        intent.putExtra(EXTRA_PREFERENCE_VALUE, (String) val);
      } else if (val instanceof Integer) {
        intent.putExtra(EXTRA_PREFERENCE_VALUE, (Integer) val);
      } else if (val instanceof Long) {
        intent.putExtra(EXTRA_PREFERENCE_VALUE, (Long) val);
      } else if (val instanceof Boolean) {
        intent.putExtra(EXTRA_PREFERENCE_VALUE, (Boolean) val);
      }
      context.sendBroadcast(intent);
    }
  }
}
