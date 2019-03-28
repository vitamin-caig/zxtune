/**
 *
 * @file
 *
 * @brief Playback service interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import android.net.Uri;

public interface PlaybackService {

  void setNowPlaying(Uri uri);
  
  PlaylistControl getPlaylistControl();
  
  PlaybackControl getPlaybackControl();
  
  SeekControl getSeekControl();
  
  Visualizer getVisualizer();
  
  void subscribe(Callback cb);
  void unsubscribe(Callback cb);
}
