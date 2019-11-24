/**
 * @file
 * @brief Player events listener interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.sound;

import androidx.annotation.NonNull;

/**
 * All calls are synchronous and may be performed from background thread
 */
public interface PlayerEventsListener {

  /**
   * Called when playback is started or resumed
   */
  void onStart();

  /**
   * Called when seek is started
   */
  void onSeeking();

  /**
   * Called when played stream come to an end
   */
  void onFinish();

  /**
   * Called when playback stopped (also called after onFinish)
   */
  void onStop();

  /**
   * Called on unexpected error occurred
   * @param e Exception happened
   */
  void onError(@NonNull Exception e);
}
