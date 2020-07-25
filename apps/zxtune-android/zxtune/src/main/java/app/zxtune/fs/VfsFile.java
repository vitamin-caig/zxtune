/**
 * @file
 * @brief Vfs file object interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

public interface VfsFile extends VfsObject {

  /**
   * @return File size in implementation-specific units
   */
  String getSize();
}
