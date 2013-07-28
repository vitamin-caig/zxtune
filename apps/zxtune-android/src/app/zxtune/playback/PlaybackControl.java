/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

public interface PlaybackControl {

  /*
   * Activate currently playing item
   */
  public void play();
  
  /*
   * Stop currently playing item
   */
  public void stop();
  
  /*
   * @return true if something is played now
   */
  public boolean isPlaying();
  
  /*
   * Play next item in sequence
   */
  public void next();
  
  /*
   * Play previous item in sequence
   */
  public void prev();
}
