/**
 *
 * @file
 *
 * @brief  References iterator interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playlist;

import app.zxtune.TimeStamp;

public interface ReferencesIterator {

  class Entry {

    public String location;

    public String title = "";
    public String author = "";
    public TimeStamp duration = TimeStamp.EMPTY;
  }

  /**
   * @return Current position entry
   */
  Entry getItem();
  
  /**
   * @return true if moved to next position
   */
  boolean next();
  
  /**
   * @return true if moved to previous position
   */
  boolean prev();
  
  /**
   * @return false if underlying list is empty
   */
  boolean first();
  
  /**
   * @return false if underlying list is empty
   */
  boolean last();
  
  /**
   * @return false if underlying list is empty
   */
  boolean random();
}
