/**
 *
 * @file
 *
 * @brief Playback controller interface
 *
 * @author vitamin.caig@gmail.com
 *
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
  
  /*
   * @return true if playback is looped
   */
  public boolean isLooped();
  
  /*
   * Set loop mode
   */
  public void setLooped(boolean looped);
}
