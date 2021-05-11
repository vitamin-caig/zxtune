/**
 * @file
 * @brief Preferences activity
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import android.app.Application;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.annotation.XmlRes;
import androidx.appcompat.app.AppCompatActivity;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.ViewModelProvider;
import androidx.preference.Preference;
import androidx.preference.PreferenceDataStore;
import androidx.preference.PreferenceFragmentCompat;

import app.zxtune.preferences.DataStore;

public class PreferencesActivity extends AppCompatActivity implements PreferenceFragmentCompat.OnPreferenceStartFragmentCallback {

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
      getPreferenceManager().setPreferenceDataStore(Model.of(this).getStore());
      setPreferencesFromResource(layout, rootKey);
    }
  }

  // public for provider
  public static class Model extends AndroidViewModel {

    private final PreferenceDataStore store;

    static Model of(androidx.fragment.app.Fragment owner) {
      return new ViewModelProvider(owner.requireActivity(),
          ViewModelProvider.AndroidViewModelFactory.getInstance(owner.getActivity().getApplication())).get(Model.class);
    }

    public Model(Application app) {
      super(app);
      store = new DataStore(app);
    }

    final PreferenceDataStore getStore() {
      return store;
    }
  }
}
