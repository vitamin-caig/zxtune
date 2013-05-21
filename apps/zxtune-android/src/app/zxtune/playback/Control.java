/**
 * @file
 * @brief Playback control interface
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */
package app.zxtune.playback;

import android.net.Uri;
import app.zxtune.TimeStamp;

/**
 *  Playback control interface
 */
public interface Control {

  /*
   * @return Currently playing item or null if not selected
   */
  public Item getItem();
  
  /*
   * @return Currently playing item's position or null if stopped
   */
  public TimeStamp getPlaybackPosition();
  
  /*
   * @return Array of spectrum analysis (256 * level + band)  
   */
  public int[] getSpectrumAnalysis();

  /*
   * @return true if now playing
   */
  public boolean isPlaying();
  
  /*
   * Start playback of specified item (data or playlist)
   */
  public void play(Uri item);
  
  /*
   * Start/continue playback of current item
   */
  public void play();

  /*
   * Stop playing or do nothing
   */
  public void stop();
  
  /*
   * Change playback position
   */
  public void setPlaybackPosition(TimeStamp pos);
}
