/**
 * @file
 * @brief Notification callback interface
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */
package app.zxtune.playback;

public interface Callback {

  /**
   * Called on control interface changing (connect/disconnect etc)
   */
  public void onControlChanged(Control control);
  
  /**
   * Called on status change (all changes before connection are lost)
   */
  public void onStatusChanged(Status status);

  /**
   * Called on active item change
   */
  public void onItemChanged(Item item);
}
