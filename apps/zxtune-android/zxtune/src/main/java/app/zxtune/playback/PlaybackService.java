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

  public Item getNowPlaying();
  
  public void setNowPlaying(Uri[] uris);
  
  public PlaylistControl getPlaylistControl();
  
  public PlaybackControl getPlaybackControl();
  
  public SeekControl getSeekControl();
  
  public Visualizer getVisualizer();
  
  public void subscribe(Callback cb);
  public void unsubscribe(Callback cb);
}
