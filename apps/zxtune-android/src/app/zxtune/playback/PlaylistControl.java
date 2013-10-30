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
}
