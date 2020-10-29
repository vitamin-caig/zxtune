/**
 * @file
 * @brief Dialog fragment with information about program
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.app.Application;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Configuration;
import android.database.Cursor;
import android.os.Build;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.SimpleExpandableListAdapter;

import androidx.annotation.Nullable;
import androidx.collection.ArrayMap;
import androidx.collection.SparseArrayCompat;
import androidx.databinding.DataBindingUtil;
import androidx.fragment.app.DialogFragment;
import androidx.fragment.app.Fragment;
import androidx.lifecycle.AndroidViewModel;
import androidx.lifecycle.LiveData;
import androidx.lifecycle.MutableLiveData;
import androidx.lifecycle.Observer;
import androidx.lifecycle.ViewModelProvider;

import java.util.ArrayList;
import java.util.Locale;

import app.zxtune.BuildConfig;
import app.zxtune.PluginsProvider;
import app.zxtune.R;
import app.zxtune.databinding.AboutBinding;

public class AboutFragment extends DialogFragment {

  @Nullable
  private AboutBinding binding;

  public static DialogFragment createInstance() {
    return new AboutFragment();
  }

  @Override
  public View onCreateView(LayoutInflater inflater, @Nullable ViewGroup container,
                           @Nullable Bundle savedInstanceState) {

    binding = DataBindingUtil.inflate(inflater, R.layout.about, container, false);

    binding.aboutTitle.setText(getApplicationInfo());
    binding.aboutSystem.setText(getSystemInfo());

    binding.aboutPager.setAdapter(new ViewPagerAdapter(binding.aboutPager));
    Model.of(this).getData().observe(this, this::fillPlugins);

    return binding.getRoot();
  }

  @Override
  public void onStart() {
    super.onStart();

    Model.of(this).populate();
  }

  private void fillPlugins(SparseArrayCompat<ArrayList<String>> data) {

    final String TYPE = "type";
    final String DESCR = "descr";

    final ArrayList<ArrayMap<String, String>> groups = new ArrayList<>(data.size());
    final ArrayList<ArrayList<ArrayMap<String, String>>> children = new ArrayList<>();

    for (int idx = 0; idx < data.size(); ++idx) {
      final int type = data.keyAt(idx);
      groups.add(makeRowMap(TYPE, getPluginTypeString(type)));

      {
        final ArrayList<ArrayMap<String, String>> child = new ArrayList<>();
        for (String plugin : data.valueAt(idx)) {
          child.add(makeRowMap(DESCR, "\t" + plugin));
        }
        children.add(child);
      }
    }

    final String[] groupFrom = {TYPE};
    final int[] groupTo = {android.R.id.text1};
    final String[] childFrom = {DESCR};
    final int[] childTo = {android.R.id.text1};
    final SimpleExpandableListAdapter adapter = new SimpleExpandableListAdapter(getActivity(),
        groups, android.R.layout.simple_expandable_list_item_1, groupFrom, groupTo,
        children, android.R.layout.simple_list_item_1, childFrom, childTo);
    binding.aboutPlugins.setAdapter(adapter);
  }

  private String getPluginTypeString(int type) {
    final int resId = PluginsProvider.Types.values()[type].nameId();
    return getString(resId);
  }

  private static ArrayMap<String, String> makeRowMap(String key, String value) {
    final ArrayMap<String, String> result = new ArrayMap<>();
    result.put(key, value);
    return result;
  }

  // public for provider
  public static class Model extends AndroidViewModel {

    // resId => [description]
    private final MutableLiveData<SparseArrayCompat<ArrayList<String>>> data = new MutableLiveData<>();

    static Model of(Fragment owner) {
      return new ViewModelProvider(owner,
          ViewModelProvider.AndroidViewModelFactory.getInstance(owner.getActivity().getApplication())).get(Model.class);
    }

    public Model(Application app) {
      super(app);
    }

    LiveData<SparseArrayCompat<ArrayList<String>>> getData() {
      return data;
    }

    final void populate() {
      if (data.getValue() == null) {
        load();
      }
    }

    private void load() {
      final ContentResolver resolver = getApplication().getContentResolver();
      final SparseArrayCompat<ArrayList<String>> result = new SparseArrayCompat<>();
      final Cursor cursor = resolver.query(PluginsProvider.getUri(), null, null, null, null);
      if (cursor != null) {
        try {
          while (cursor.moveToNext()) {
            final int type = cursor.getInt(PluginsProvider.Columns.Type.ordinal());
            final String descr = cursor.getString(PluginsProvider.Columns.Description.ordinal());
            if (!result.containsKey(type)) {
              result.append(type, new ArrayList<String>());
            }
            result.get(type, null).add(descr);
          }
        } finally {
          cursor.close();
        }
      }
      data.postValue(result);
    }
  }

  private String getApplicationInfo() {
    return String.format(Locale.US, "%s b%d (%s)", getString(R.string.app_name),
        BuildConfig.VERSION_CODE, BuildConfig.FLAVOR);
  }

  private String getSystemInfo() {
    final InfoBuilder builder = new InfoBuilder(getActivity());
    builder.buildOSInfo();
    builder.buildConfigurationInfo();
    return builder.getResult();
  }

  private static class InfoBuilder {

    private final StringBuilder strings;
    private final Context context;

    InfoBuilder(Context context) {
      this.strings = new StringBuilder();
      this.context = context;
    }

    final void buildOSInfo() {
      addString(String.format(Locale.US, "Android %s (API v%d)", Build.VERSION.RELEASE, Build.VERSION.SDK_INT));
      if (Build.VERSION.SDK_INT >= 21) {
        addString(String.format(Locale.US, "%s (%s)", Build.MODEL, TextUtils.join("/",
            Build.SUPPORTED_ABIS)));
      } else {
        addString(String.format(Locale.US, "%s (%s/%s)", Build.MODEL, Build.CPU_ABI, Build.CPU_ABI2));
      }
    }

    final void buildConfigurationInfo() {
      final Configuration config = context.getResources().getConfiguration();
      final DisplayMetrics metrics = context.getResources().getDisplayMetrics();
      addWord(getLayoutSize(config.screenLayout));
      addWord(getLayoutRatio(config.screenLayout));
      addWord(getOrientation(config.orientation));
      addWord(getDensity(metrics.densityDpi));
      addWord(config.locale.toString());
    }

    private void addString(String s) {
      strings.append(s);
      strings.append('\n');
    }

    private void addWord(String s) {
      strings.append(s);
      strings.append(' ');
    }

    private static String getLayoutSize(int layout) {
      switch (layout & Configuration.SCREENLAYOUT_SIZE_MASK) {
        case Configuration.SCREENLAYOUT_SIZE_XLARGE:
          return "xlarge";
        case Configuration.SCREENLAYOUT_SIZE_LARGE:
          return "large";
        case Configuration.SCREENLAYOUT_SIZE_NORMAL:
          return "normal";
        case Configuration.SCREENLAYOUT_SIZE_SMALL:
          return "small";
        default:
          return "size-undef";
      }
    }

    private static String getLayoutRatio(int layout) {
      switch (layout & Configuration.SCREENLAYOUT_LONG_MASK) {
        case Configuration.SCREENLAYOUT_LONG_YES:
          return "long";
        case Configuration.SCREENLAYOUT_LONG_NO:
          return "notlong";
        default:
          return "ratio-undef";
      }
    }

    private static String getOrientation(int orientation) {
      switch (orientation) {
        case Configuration.ORIENTATION_LANDSCAPE:
          return "land";
        case Configuration.ORIENTATION_PORTRAIT:
          return "port";
        case Configuration.ORIENTATION_SQUARE:
          return "square";
        default:
          return "orientation-undef";
      }
    }

    private static String getDensity(int density) {
      switch (density) {
        case DisplayMetrics.DENSITY_LOW:
          return "ldpi";
        case DisplayMetrics.DENSITY_MEDIUM:
          return "mdpi";
        case DisplayMetrics.DENSITY_HIGH:
          return "hdpi";
        case DisplayMetrics.DENSITY_XHIGH:
          return "xhdpi";
        case 480/*DisplayMetrics.DENSITY_XXHIGH APIv16+ */:
          return "xxhdpi";
        case 640/*DisplayMetrics.DENSITY_XXXHIGH APIv18+ */:
          return "xxxhdpi";
        default:
          return String.format(Locale.US, "%ddpi", density);
      }
    }

    final String getResult() {
      return strings.toString();
    }
  }
}
