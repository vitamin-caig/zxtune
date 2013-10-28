/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs;

import java.io.IOException;
import java.nio.ByteBuffer;

public interface VfsFile extends VfsObject {

  /**
   * @return File size in implementation-specific units
   */
  public String getSize();
  
  /**
   * @return Raw file content
   */
  public ByteBuffer getContent() throws IOException;
}
