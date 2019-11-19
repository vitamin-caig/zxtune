/**
 *
 * @file
 *
 * @brief Dialog fragment with information about program
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui;

import android.app.Activity;
import android.app.Dialog;
import android.content.Context;
import android.content.res.Configuration;
import android.database.Cursor;
import android.os.Build;
import android.os.Bundle;
import androidx.fragment.app.DialogFragment;
import androidx.loader.app.LoaderManager;
import androidx.loader.content.CursorLoader;
import androidx.loader.content.Loader;
import androidx.collection.ArrayMap;
import androidx.viewpager.widget.ViewPager;
import androidx.appcompat.app.AlertDialog;
import android.text.TextUtils;
import android.util.DisplayMetrics;
import android.view.View;
import android.widget.ExpandableListView;
import android.widget.SimpleExpandableListAdapter;
import android.widget.TextView;
import app.zxtune.BuildConfig;
import app.zxtune.PluginsProvider;
import app.zxtune.R;

import java.util.ArrayList;
import java.util.Locale;

public class AboutFragment extends DialogFragment {

  public static DialogFragment createInstance() {
    return new AboutFragment();
  }

  @Override
  public Dialog onCreateDialog(Bundle savedInstanceState) {
    final Context ctx = getActivity();

    final View view = ((Activity) ctx).getLayoutInflater().inflate(R.layout.about, null, false);
    ((TextView) view.findViewById(R.id.about_system)).setText(getSystemInfo());
    final ViewPager pager = view.findViewById(R.id.about_pager);
    pager.setAdapter(new ViewPagerAdapter(pager));
    final ExpandableListView plugins = view.findViewById(R.id.about_plugins);
    fillPlugins(plugins);

    final AlertDialog result = new AlertDialog.Builder(ctx)
        .setTitle(getApplicationInfo())
        .setView(view)
        .create();
    return result;
  }

  private void fillPlugins(final ExpandableListView plugins) {
    getLoaderManager().initLoader(0, null, new LoaderManager.LoaderCallbacks<Cursor>() {

      @Override
      public Loader<Cursor> onCreateLoader(int arg0, Bundle arg1) {
        return new CursorLoader(getActivity(), PluginsProvider.getUri(), null, null, null, null);
      }

      @Override
      public void onLoadFinished(Loader<Cursor> loader, Cursor cursor) {
        final Context ctx = getActivity();
        final ArrayList<ArrayMap<String, String>> groups = new ArrayList<>();
        final ArrayList<ArrayList<ArrayMap<String, String>>> children = new ArrayList<>();
        for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
          final int type = cursor.getInt(PluginsProvider.Columns.Type.ordinal());
          final String descr = cursor.getString(PluginsProvider.Columns.Description.ordinal());
          while (type >= groups.size()) {
            groups.add(new ArrayMap<String, String>());
            children.add(new ArrayList<ArrayMap<String, String>>());
          }
          final ArrayMap<String, String> plugin = new ArrayMap<>(1);
          plugin.put(PluginsProvider.Columns.Description.name(), "\t" + descr);
          children.get(type).add(plugin);
        }
        for (int type = 0; type != groups.size(); ++type) {
          groups.get(type).put(PluginsProvider.Columns.Type.name(), ctx.getString(getPluginTypeString(type)));
        }
        
        final String[] groupFrom = {PluginsProvider.Columns.Type.name()};
        final int[] groupTo = {android.R.id.text1};
        final String[] childFrom = {PluginsProvider.Columns.Description.name()};
        final int[] childTo = {android.R.id.text1};
        final SimpleExpandableListAdapter adapter = new SimpleExpandableListAdapter(getActivity(), 
            groups, android.R.layout.simple_expandable_list_item_1, groupFrom, groupTo,
                children, android.R.layout.simple_list_item_1, childFrom, childTo);
        
        plugins.setAdapter(adapter);
      }

      @Override
      public void onLoaderReset(Loader<Cursor> arg0) {
      }
    });
  }

  private static int getPluginTypeString(int type) {
    return PluginsProvider.Types.values()[type].nameId();
  }
  
  private String getApplicationInfo() {
    return String.format(Locale.US, "%s b%d (%s)", getActivity().getString(R.string.app_name),
            BuildConfig.VERSION_CODE, BuildConfig.VERSION_NAME);
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

    public void buildOSInfo() {
      addString(String.format("Android %s (API v%d)", Build.VERSION.RELEASE, Build.VERSION.SDK_INT));
      if (Build.VERSION.SDK_INT >= 21) {
        addString(String.format("%s (%s)", Build.MODEL, TextUtils.join("/", Build.SUPPORTED_ABIS)));
      } else {
        addString(String.format("%s (%s/%s)", Build.MODEL, Build.CPU_ABI, Build.CPU_ABI2));
      }
    }

    public void buildConfigurationInfo() {
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

    @SuppressWarnings("deprecation")
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

    public final String getResult() {
      return strings.toString();
    }
  }
}
