/**
 *
 * @file
 *
 * @brief ListView tools
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui.utils;

import android.widget.ListView;

import app.zxtune.R;

public class ListViewTools {

  public static void storeViewPosition(ListView view, int pos) {
    view.setTag(pos);
  }
  
  public static void useStoredViewPosition(ListView view) {
    final Integer pos = (Integer) view.getTag();
    if (pos != null) {
      view.setSelection(pos);
      view.setTag(null);
    }
  }

  public static boolean processActionItemClick(ListView view, int itemId) {
    switch (itemId) {
      case R.id.action_select_all:
        selectAll(view);
        break;
      case R.id.action_select_none:
        selectNone(view);
        break;
      case R.id.action_select_invert:
        invertSelection(view);
        break;
      default:
        return false;
    }
    return true;
  }

  private static void selectAll(ListView view) {
    for (int i = 0, lim = view.getAdapter().getCount(); i != lim; ++i) {
      view.setItemChecked(i, true);
    }
  }

  private static void selectNone(ListView view) {
    for (int i = 0, lim = view.getAdapter().getCount(); i != lim; ++i) {
      view.setItemChecked(i, false);
    }
  }

  private static void invertSelection(ListView view) {
    for (int i = 0, lim = view.getAdapter().getCount(); i != lim; ++i) {
      view.setItemChecked(i, !view.isItemChecked(i));
    }
  }
}
