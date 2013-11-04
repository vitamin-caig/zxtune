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

public interface ReferencesIterator {

  public static class Entry {

    public String location;
    
    //TODO
    //HashMap<String, Object> properties;
  }

  /**
   * @return Current position entry
   */
  public Entry getItem();
  
  /**
   * @return true if moved to next position
   */
  public boolean next();
  
  /**
   * @return true if moved to previous position
   */
  public boolean prev();
  
  /**
   * @return false if underlying list is empty
   */
  public boolean first();
  
  /**
   * @return false if underlying list is empty
   */
  public boolean last();
  
  /**
   * @return false if underlying list is empty
   */
  public boolean random();
}
