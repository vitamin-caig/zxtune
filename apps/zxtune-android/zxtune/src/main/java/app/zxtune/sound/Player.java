/**
 * @file
 * @brief Sound player interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.sound;

import android.support.annotation.NonNull;

/**
 * Base interface for low-level sound player
 */
public interface Player {

  void setSource(@NonNull SamplesSource src) throws Exception;

  void startPlayback();

  void stopPlayback();

  boolean isStarted();

  void release();
}
