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
   * @param bands Maximal count of bands to analyze
   * @return Array of spectrum analysis 
   */
  public int[] getSpectrumAnalysis(int bands);

  /*
   * @return Current playback status
   */
  public Status getStatus();
  
  /*
   * Start playback of specified item (data or playlist)
   */
  public void play(Uri item);

  /*
   * Continue previously played or do nothing if already played
   */
  public void play();

  /*
   * Pause playing or do nothing
   */
  public void pause();

  /*
   * Stop playing or do nothing
   */
  public void stop();
}
