package app.zxtune.ui.utils;

import android.view.View;
import android.view.ViewGroup;

public final class UiUtils {
  public static void setViewEnabled(View view, boolean enabled) {
    if (view instanceof ViewGroup) {
      final ViewGroup group = (ViewGroup) view;
      for (int idx = 0, lim = group.getChildCount(); idx != lim; ++idx) {
        setViewEnabled(group.getChildAt(idx), enabled);
      }
    } else {
      view.setEnabled(enabled);
    }
  }
}
