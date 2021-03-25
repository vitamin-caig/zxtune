/**
 * @file
 * @brief Vfs directory object interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import java.io.IOException;

import app.zxtune.utils.ProgressCallback;

public interface VfsDir extends VfsObject {

  /**
   * Directory content enumerating callback
   */
  abstract class Visitor implements ProgressCallback {

    /**
     * Called on visited directory
     */
    public abstract void onDir(VfsDir dir);

    /**
     * Called on visited file
     */
    public abstract void onFile(VfsFile file);

    /**
     * Called when items count is known (at any moment, maybe approximate)
     */
    public void onItemsCount(int count) {}

    @Override
    public void onProgressUpdate(int done, int total) {}
  }

  /**
   * Enumerate directory content
   *
   * @param visitor Callback
   */
  void enumerate(Visitor visitor) throws IOException;
}
