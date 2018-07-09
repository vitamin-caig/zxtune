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

  void startPlayback();

  void stopPlayback();

  void pausePlayback();
  
  boolean isStarted();

  boolean isPaused();
  
  void release();
}
