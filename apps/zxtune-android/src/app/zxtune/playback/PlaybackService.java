/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

import android.net.Uri;
import app.zxtune.Releaseable;

public interface PlaybackService {

  public Item getNowPlaying();
  
  public void setNowPlaying(Uri uri);
  
  public PlaybackControl getPlaybackControl();
  
  public SeekControl getSeekControl();
  
  public Visualizer getVisualizer();
  
  public Releaseable subscribeForEvents(Callback callback);
}
