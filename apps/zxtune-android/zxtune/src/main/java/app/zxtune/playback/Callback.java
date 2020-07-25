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

import app.zxtune.TimeStamp;

public interface Callback {

  /**
   * Called after subscription
   */
  void onInitialState(PlaybackControl.State state);

  /**
   * Called on new state (may be the same)
   */
  void onStateChanged(PlaybackControl.State state, TimeStamp pos);

  /**
   * Called on active item change
   */
  void onItemChanged(Item item);

  /**
   * Called on error happends
   */
  void onError(String e);
}
