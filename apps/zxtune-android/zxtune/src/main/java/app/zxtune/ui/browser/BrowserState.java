/**
 *
 * @file
 *
 * @brief File browser state helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui.browser;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.net.Uri;

import java.util.Locale;

class BrowserState {
  
  private final static String PREF_BROWSER = "browser_";
  private final static String PREF_BROWSER_CURRENT = PREF_BROWSER + "current";
  private final static String PREF_BROWSER_PATH_TEMPLATE = PREF_BROWSER + "%d_path";
  private final static String PREF_BROWSER_VIEWPOS_TEMPLATE = PREF_BROWSER + "%d_viewpos";
  private final SharedPreferences prefs;
  private PathAndPosition current;
  
  BrowserState(SharedPreferences prefs) {
    this.prefs = prefs;
    this.current = new PathAndPosition();
  }
  
  final Uri getCurrentPath() {
    return current.getPath();
  }
  
  final void setCurrentPath(Uri uri) {
    final Uri curPath = current.getPath();

    if (curPath.equals(uri)) {
      return;
    } else if (isNested(curPath, uri)) {
      setInnerPath(uri);
    } else if (isNested(uri, curPath)) {
      setOuterPath(uri);
    } else {
      setPath(uri);
    }
    current.store();
  }
  
  final int getCurrentViewPosition() {
    return current.getViewPosition();
  }
  
  final void setCurrentViewPosition(int pos) {
    current.setViewPosition(pos);
  }
  
  private void setInnerPath(Uri newPath) {
    current = new PathAndPosition(current.getIndex() + 1, newPath);
  }
  
  private void setOuterPath(Uri newPath) {
    current = findByPath(newPath);
  }
  
  private void setPath(Uri newPath) {
    current = new PathAndPosition(0, newPath);
  }
  
  private PathAndPosition findByPath(Uri newPath) {
    for (int idx = current.getIndex() - 1; idx >= 0; --idx) {
      final PathAndPosition pos = new PathAndPosition(idx);
      if (newPath.equals(pos.getPath())) {
        return pos;
      }
    }
    return new PathAndPosition(0, newPath);
  }
  
  // checks if rh is nested path relative to lh
  private static boolean isNested(Uri lh, Uri rh) {
    final String lhScheme = lh.getScheme();
    if (lhScheme != null && lhScheme.equals(rh.getScheme())) {
      final String rhPath = rh.getPath();
      final String lhPath = lh.getPath();
      if (rhPath != null) {
        if (lhPath == null) {
          return true;
        } else if (lhPath.equals(rhPath)) {
          return isNestedSubpath(lh.getFragment(), rh.getFragment());
        } else {
          return rhPath.startsWith(lhPath);
        }
      }
    }
    return false;
  }
  
  private static boolean isNestedSubpath(String lh, String rh) {
    return rh != null && (lh == null || rh.startsWith(lh));
  }
  
  private class PathAndPosition {
    
    private final int index;
    private final Uri path;
    private int position;
    
    PathAndPosition() {
      this(prefs.getInt(PREF_BROWSER_CURRENT, 0));
    }
    
    PathAndPosition(int idx) {
      this.index = idx;
      this.path = Uri.parse(prefs.getString(getPathKey(), ""));
      this.position = prefs.getInt(getPosKey(), 0);
    }
    
    PathAndPosition(int idx, Uri path) {
      this.index = idx;
      this.path = path;
      this.position = 0;
    }
    
    final int getIndex() {
      return index;
    }
    
    final Uri getPath() {
      return path;
    }
    
    final int getViewPosition() {
      return position;
    }
    
    final void setViewPosition(int newPos) {
      if (newPos != position) {
        position = newPos;
        store();
      }
    }
    
    private void store() {
      final Editor editor = prefs.edit();
      editor.putString(getPathKey(), this.path.toString());
      editor.putInt(getPosKey(), this.position);
      editor.putInt(PREF_BROWSER_CURRENT, index);
      editor.apply();
    }
    
    private String getPathKey() {
      return String.format(Locale.US, PREF_BROWSER_PATH_TEMPLATE, index);
    }
    
    private String getPosKey() {
      return String.format(Locale.US, PREF_BROWSER_VIEWPOS_TEMPLATE, index);
    }
  }
}
