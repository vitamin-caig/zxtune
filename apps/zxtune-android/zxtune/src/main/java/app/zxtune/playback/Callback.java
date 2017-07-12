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

public interface Callback {

  /**
   * Called on state change (all changes before connection are lost)
   */
  public void onStateChanged(PlaybackControl.State state);

  /**
   * Called on active item change
   */
  public void onItemChanged(Item item);
  
  /**
   * Called on I/O operation status change
   */
  public void onIOStatusChanged(boolean isActive);
  
  /**
   * Called on error happends
   */
  public void onError(String e);
}
