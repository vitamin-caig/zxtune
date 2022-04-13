/**
 * @file
 * @brief Iterators factory
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playback;

import android.content.Context;
import android.net.Uri;

import androidx.preference.PreferenceDataStore;

import java.io.IOException;

import app.zxtune.playback.PlaybackControl.SequenceMode;
import app.zxtune.playlist.PlaylistQuery;
import app.zxtune.preferences.DataStore;

public final class IteratorFactory {

  /**
   *
   * @param context Operational context
   * @param uri Start point of playback
   * @return new iterator
   * @throws IOException
   */
  public static Iterator createIterator(Context context, Uri uri) throws Exception {
    if (PlaylistQuery.isPlaylistUri(uri)) {
      return new PlaylistIterator(context, uri);
    } else {
      return FileIterator.create(context, uri);
    }
  }

  //TODO: implement abstract iterator-typed visitor and preferences notification 
  public static class NavigationMode {

    private static final String KEY = "playlist.navigation_mode";
    private final PreferenceDataStore prefs;

    public NavigationMode(Context context) {
      this(new DataStore(context));
    }

    public NavigationMode(PreferenceDataStore prefs) {
      this.prefs = prefs;
    }

    public final void set(SequenceMode mode) {
      prefs.putString(KEY, mode.toString());
    }

    public final SequenceMode get() {
      final String stored = prefs.getString(KEY, null);
      return stored != null ? SequenceMode.valueOf(stored) : SequenceMode.ORDERED;
    }
  }

  private IteratorFactory() {
  }
}
