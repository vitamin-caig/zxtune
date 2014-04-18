/**
 *
 * @file
 *
 * @brief Vfs directory object interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs;

import java.io.IOException;

public interface VfsDir extends VfsObject {

  /**
   * Directory content enumerating callback
   */
  public interface Visitor {
    
    /**
     * Called when items count is known (at any moment, maybe approximate)
     */
    public void onItemsCount(int count);
    
    /**
     * Called on visited directory
     */
    public void onDir(VfsDir dir);

    /**
     * Called on visited file
     */
    public void onFile(VfsFile file);
  }
  
  /**
   * Retrieve parent directory or null
   */
  public VfsDir getParent();
  
  /**
   * Enumerate directory content
   * @param visitor Callback
   */
  public void enumerate(Visitor visitor) throws IOException;

  /**
   * Search for objects matching criteria
   * @param mask Part of object name
   * @param visitor Callback
   */
  public void find(String mask, Visitor visitor);
}
