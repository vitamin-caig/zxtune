/**
 *
 * @file
 *
 * @brief Iterators factory
 *
 * @author vitamin.caig@gmail.com
 *
 */
package app.zxtune.playback;

import android.content.ContentResolver;
import android.content.Context;
import android.content.SharedPreferences;
import android.net.Uri;

import java.io.IOException;

import app.zxtune.Preferences;
import app.zxtune.playback.PlaybackControl.SequenceMode;

public final class IteratorFactory {

  private static boolean isPlaylistUri(Uri uri) {
    return ContentResolver.SCHEME_CONTENT.equals(uri.getScheme());
  }
  
  /**
   * 
   * @param context Operational context
   * @param uris List of objects identifiers. In case of multiple, used as files/folders identifiers 
   * @return new iterator
   * @throws IOException
   */
  public static Iterator createIterator(Context context, Uri[] uris) throws IOException {
    final Uri first = uris[0];
    if (isPlaylistUri(first)) {
      return new PlaylistIterator(context, first);
    } else {
      return new FileIterator(context, uris);
    }
  }
  
  //TODO: implement abstract iterator-typed visitor and preferences notification 
  static class NavigationMode {
    
    private static final String KEY = "playlist.navigation_mode";
    private final SharedPreferences prefs;
    
    NavigationMode(Context context) {
      this.prefs = Preferences.getDefaultSharedPreferences(context);
    }
    
    final void set(SequenceMode mode) {
      prefs.edit().putString(KEY, mode.toString()).apply();
    }
    
    final SequenceMode get() {
      return SequenceMode.valueOf(prefs.getString(KEY, SequenceMode.ORDERED.toString()));
    }
  }
  
  private IteratorFactory() {
  }
}
