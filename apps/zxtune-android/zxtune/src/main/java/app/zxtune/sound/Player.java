/**
 * @file
 * @brief Sound player interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.sound;

/**
 * Base interface for low-level sound player
 */
public interface Player {

  void startPlayback();

  void stopPlayback();

  boolean isStarted();

  void release();
}
