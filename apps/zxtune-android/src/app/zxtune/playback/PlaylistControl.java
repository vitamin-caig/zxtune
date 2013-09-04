/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

import android.net.Uri;

/**
 * Playlist-related control functionality interface
 */
public interface PlaylistControl {

  public void add(Uri uri);
  
  public void delete(Uri uri);
}
