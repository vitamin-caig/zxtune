/**
 *
 * @file
 *
 * @brief Playback item extension describing also module
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;

import app.zxtune.Releaseable;
import app.zxtune.core.Module;

public interface PlayableItem extends Item, Releaseable {

  /**
   * @return Associated module
   */
  public Module getModule();
}
