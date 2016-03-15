/**
 *
 * @file
 *
 * @brief Sound player interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.sound;

/**
 * Base interface for low-level sound player
 */
public interface Player {

  public void startPlayback();

  public void stopPlayback();
  
  public boolean isStarted();
  
  public void release();
}
