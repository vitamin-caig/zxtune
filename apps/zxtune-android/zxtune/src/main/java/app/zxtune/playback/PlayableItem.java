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

import app.zxtune.core.Module;

public interface PlayableItem extends Item {

  /**
   * @return Associated module
   */
  Module getModule();
}
