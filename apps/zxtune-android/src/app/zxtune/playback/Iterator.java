/**
 *
 * @file
 *
 * @brief Playback iterator interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playback;


public interface Iterator {

  /**
   *  Retrieve current position' item
   *  @return Newly loaded item
   */
  PlayableItem getItem();

  /**
   *  Move to next position
   *  @return true if successfully moved, else getItem will return the same item
   */
  boolean next();
  
  /**
   *  Move to previous position
   *  @return true if successfully moved, else getItem will return the same item
   */
  boolean prev();
}
