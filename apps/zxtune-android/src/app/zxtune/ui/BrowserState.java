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
  
  public BrowserState(SharedPreferences prefs) {
    this.prefs = prefs;
    this.current = new PathAndPosition();
  }
  
  public Uri getCurrentPath() {
    return Uri.parse(current.getPath());
  }
  
  public void setCurrentPath(Uri uri) {
    final String curPath = current.getPath();
    final String newPath = uri.toString();
    if (curPath.equals(newPath)) {
      return;
    } else if (isNested(curPath, newPath)) {
      setInnerPath(newPath);
    } else if (isNested(newPath, curPath)) {
      setOuterPath(newPath);
    } else {
      setPath(newPath);
    }
    current.store();
  }
  
  public int getCurrentViewPosition() {
    return current.getViewPosition();
  }
  
  public void setCurrentViewPosition(int pos) {
    current.setViewPosition(pos);
  }
  
  private void setInnerPath(String newPath) {
    current = new PathAndPosition(current.getIndex() + 1, newPath);
  }
  
  private void setOuterPath(String newPath) {
    current = findByPath(newPath);
  }
  
  private void setPath(String newPath) {
    current = new PathAndPosition(0, newPath);
  }
  
  private PathAndPosition findByPath(String newPath) {
    for (int idx = current.getIndex() - 1; idx >= 0; --idx) {
      final PathAndPosition pos = new PathAndPosition(idx);
      if (newPath.equals(pos.getPath())) {
        return pos;
      }
    }
    return new PathAndPosition(0, newPath);
  }
  
  // checks if rh is nested path relative to lh
  private static final boolean isNested(String lh, String rh) {
    return rh.startsWith(lh);
  }
  
  private class PathAndPosition {
    
    private final int index;
    private String path;
    private int position;
    
    public PathAndPosition() {
      this(prefs.getInt(PREF_BROWSER_CURRENT, 0));
    }
    
    public PathAndPosition(int idx) {
      this.index = idx;
      this.path = prefs.getString(getPathKey(), "");
      this.position = prefs.getInt(getPosKey(), 0);
    }
    
    public PathAndPosition(int idx, String path) {
      this.index = idx;
      this.path = path;
      this.position = 0;
    }
    
    public int getIndex() {
      return index;
    }
    
    public String getPath() {
      return path;
    }
    
    public int getViewPosition() {
      return position;
    }
    
    public void setViewPosition(int newPos) {
      if (newPos != position) {
        position = newPos;
        store();
      }
    }
    
    private void store() {
      final Editor editor = prefs.edit();
      editor.putString(getPathKey(), this.path);
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
