/**
 * @file
 * @brief Vfs file object interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import java.io.IOException;
import java.nio.ByteBuffer;

public interface VfsFile extends VfsObject {

  /**
   * @return File size in implementation-specific units
   */
  String getSize();

  /**
   * @return Raw file content
   */
  ByteBuffer getContent() throws IOException;
}
