/**
 * @file
 * @brief Helper for browser state storage
 * @version $Id:$
 * @author
 */
package app.zxtune.ui;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;
import android.net.Uri;

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
  
  Uri getCurrentPath() {
    return current.getPath();
  }
  
  void setCurrentPath(Uri uri) {
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
  
  int getCurrentViewPosition() {
    return current.getViewPosition();
  }
  
  void setCurrentViewPosition(int pos) {
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
  private static final boolean isNested(Uri lh, Uri rh) {
    final String cleanLh = lh.buildUpon().clearQuery().build().toString();
    final String cleanRh = rh.buildUpon().clearQuery().build().toString();
    return cleanRh.startsWith(cleanLh);
  }
  
  private class PathAndPosition {
    
    private final int index;
    private Uri path;
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
    
    int getIndex() {
      return index;
    }
    
    Uri getPath() {
      return path;
    }
    
    int getViewPosition() {
      return position;
    }
    
    void setViewPosition(int newPos) {
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
      editor.commit();
    }
    
    private String getPathKey() {
      return String.format(PREF_BROWSER_PATH_TEMPLATE, index);
    }
    
    private String getPosKey() {
      return String.format(PREF_BROWSER_VIEWPOS_TEMPLATE, index);
    }
  }
}
