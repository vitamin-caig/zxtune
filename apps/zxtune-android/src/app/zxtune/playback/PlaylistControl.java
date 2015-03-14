/**
 *
 * @file
 *
 * @brief Playlist controller interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import android.net.Uri;

/**
 * Playlist-related control functionality interface
 */
public interface PlaylistControl {

  public void add(Uri[] uris);
  
  public void delete(long[] ids);
  
  public void deleteAll();
  
  public void move(long id, int delta);
  
  //should be name-compatible with Database
  public static enum SortBy {
    title,
    author,
    duration
  };
  
  public static enum SortOrder {
    asc,
    desc
  }
  
  public void sort(SortBy by, SortOrder order);
}
