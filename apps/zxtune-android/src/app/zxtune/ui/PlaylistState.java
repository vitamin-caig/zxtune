/**
 * @file
 * @brief Playlist view state
 * @version $Id:$
 * @author
 */
package app.zxtune.ui;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;


class PlaylistState {
  
  private final static String PREF_PLAYLIST = "playlist_";
  private final static String PREF_PLAYLIST_VIEWPOS = PREF_PLAYLIST + "viewpos";
  private final SharedPreferences prefs;

  public PlaylistState(SharedPreferences prefs) {
    this.prefs = prefs;
  }
  
  public int getCurrentViewPosition() {
    return prefs.getInt(PREF_PLAYLIST_VIEWPOS, 0);
  }
  
  public void setCurrentViewPosition(int pos) {
    final Editor editor = prefs.edit();
    editor.putInt(PREF_PLAYLIST_VIEWPOS, pos);
    editor.commit();
  }
}

