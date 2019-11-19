/**
 * @file
 * @brief Sound player interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.sound;

import androidx.annotation.NonNull;

/**
 * Base interface for low-level sound player
 */
public interface Player {

  void setSource(@NonNull SamplesSource src);

  void startPlayback();

  void stopPlayback();

  boolean isStarted();

  void release();
}
