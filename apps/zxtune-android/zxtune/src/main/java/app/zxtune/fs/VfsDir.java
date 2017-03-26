/**
 * @file
 * @brief Vfs directory object interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import java.io.IOException;

public interface VfsDir extends VfsObject {

  /**
   * Directory content enumerating callback
   */
  interface Visitor {

    /**
     * Called when items count is known (at any moment, maybe approximate)
     */
    void onItemsCount(int count);

    /**
     * Called on visited directory
     */
    void onDir(VfsDir dir);

    /**
     * Called on visited file
     */
    void onFile(VfsFile file);
  }

  /**
   * Enumerate directory content
   *
   * @param visitor Callback
   */
  void enumerate(Visitor visitor) throws IOException;

  /**
   * Find child with specified filename
   * @param name
   * @return null if nothing found
   */
  //@Nullable
  //VfsObject find( String name) throws IOException;
}
