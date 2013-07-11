/**
 * @file
 * @brief Extension of Item intended to be played or etc
 * @version $Id:$
 * @author
 */
package app.zxtune.playback;

import app.zxtune.Releaseable;
import app.zxtune.ZXTune;

public interface PlayableItem extends Item, Releaseable {

  /**
   * @return Newly created low-level player
   */
  public ZXTune.Player createPlayer();
}
