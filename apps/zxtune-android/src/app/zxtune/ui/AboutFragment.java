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

import java.util.Locale;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Configuration;
import android.os.Build;
import android.os.Bundle;
import android.support.v4.app.DialogFragment;
import android.util.DisplayMetrics;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.TextView;
import app.zxtune.R;

public class AboutFragment extends DialogFragment {

  public static DialogFragment createInstance() {
    return new AboutFragment();
  }

  @Override
  public View onCreateView(LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
    final View res = inflater.inflate(R.layout.about, container, false);
    getDialog().setTitle(getApplicationInfo());
    ((TextView) res.findViewById(R.id.about_system)).setText(getSystemInfo());
    return res;
  }

  private String getApplicationInfo() {
    try {
      final Context context = getActivity();
      final PackageInfo info =
          context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
      return String.format(Locale.US, "%s b%d (%s)", context.getString(info.applicationInfo.labelRes),
          info.versionCode, info.versionName);
    } catch (NameNotFoundException e) {
      return e.getLocalizedMessage();
    }
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
      addString(String.format("%s (%s/%s)", Build.MODEL, Build.CPU_ABI, Build.CPU_ABI2));
    }

    public void buildConfigurationInfo() {
      final Configuration config = context.getResources().getConfiguration();
      final DisplayMetrics metrics = context.getResources().getDisplayMetrics();
      addWord(getLayoutSize(config.screenLayout));
      addWord(getLayoutRatio(config.screenLayout));
      addWord(getOrientation(config.orientation));
      addWord(getDensity(metrics.densityDpi));
    }

    private void addString(String s) {
      strings.append(s);
      strings.append("\n");
    }

    private void addWord(String s) {
      strings.append(s);
      strings.append(" ");
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
        case 320/*DisplayMetrics.DENSITY_XHIGH*/:
          return "xhdpi";
        case 480/*DisplayMetrics.DENSITY_XXHIGH*/:
          return "xxhdpi";
        default:
          return String.format(Locale.US, "%ddpi", density);
      }
    }

    public final String getResult() {
      return strings.toString();
    }
  }
}
