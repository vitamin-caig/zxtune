/**
 *
 * @file
 *
 * @brief Notification callback interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import android.support.annotation.NonNull;

public interface Callback {

  /**
   * Called after subscription
   */
  void onInitialState(@NonNull PlaybackControl.State state, @NonNull Item item, boolean ioStatus);

  /**
   * Called on new state (may be the same)
   */
  void onStateChanged(@NonNull PlaybackControl.State state);

  /**
   * Called on active item change
   */
  void onItemChanged(@NonNull Item item);
  
  /**
   * Called on I/O operation status change
   */
  void onIOStatusChanged(boolean isActive);
  
  /**
   * Called on error happends
   */
  void onError(@NonNull String e);
}
