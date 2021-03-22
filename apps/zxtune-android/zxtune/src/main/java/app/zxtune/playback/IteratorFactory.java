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
   * @param uri Start point of playback
   * @return new iterator
   * @throws IOException
   */
  public static Iterator createIterator(Context context, Uri uri) throws Exception {
    if (isPlaylistUri(uri)) {
      return new PlaylistIterator(context, uri);
    } else {
      return FileIterator.create(context, uri);
    }
  }
  
  //TODO: implement abstract iterator-typed visitor and preferences notification 
  public static class NavigationMode {
    
    private static final String KEY = "playlist.navigation_mode";
    private final SharedPreferences prefs;
    
    public NavigationMode(Context context) {
      this(Preferences.getDefaultSharedPreferences(context));
    }

    public NavigationMode(SharedPreferences prefs) {
      this.prefs = prefs;
    }
    
    public final void set(SequenceMode mode) {
      prefs.edit().putString(KEY, mode.toString()).apply();
    }
    
    public final SequenceMode get() {
      final String stored = prefs.getString(KEY, null);
      return stored != null ? SequenceMode.valueOf(stored) : SequenceMode.ORDERED;
    }
  }
  
  private IteratorFactory() {
  }
}
