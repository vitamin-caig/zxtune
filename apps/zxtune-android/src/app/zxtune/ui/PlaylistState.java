/**
 *
 * @file
 *
 * @brief Playlist component state helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui;

import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

class PlaylistState {
  
  private final static String PREF_PLAYLIST = "playlist_";
  private final static String PREF_PLAYLIST_VIEWPOS = PREF_PLAYLIST + "viewpos";
  private final SharedPreferences prefs;

  PlaylistState(SharedPreferences prefs) {
    this.prefs = prefs;
  }
  
  final int getCurrentViewPosition() {
    return prefs.getInt(PREF_PLAYLIST_VIEWPOS, 0);
  }
  
  final void setCurrentViewPosition(int pos) {
    final Editor editor = prefs.edit();
    editor.putInt(PREF_PLAYLIST_VIEWPOS, pos);
    editor.commit();
  }
}

