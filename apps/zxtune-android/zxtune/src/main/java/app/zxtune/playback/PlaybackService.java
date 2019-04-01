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

public interface PlaybackService {

  PlaylistControl getPlaylistControl();
  
  PlaybackControl getPlaybackControl();
  
  SeekControl getSeekControl();
  
  Visualizer getVisualizer();
  
  void subscribe(Callback cb);
  void unsubscribe(Callback cb);
}
