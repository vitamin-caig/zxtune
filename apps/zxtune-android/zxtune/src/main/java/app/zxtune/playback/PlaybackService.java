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

import app.zxtune.Releaseable;

public interface PlaybackService {

  PlaybackControl getPlaybackControl();
  
  SeekControl getSeekControl();
  
  Visualizer getVisualizer();
  
  Releaseable subscribe(Callback cb);
}
