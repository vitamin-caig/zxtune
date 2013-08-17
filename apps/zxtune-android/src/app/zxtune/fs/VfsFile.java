/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs;

import java.io.IOException;

public interface VfsFile extends VfsObject {

  /**
   * @return File size in bytes
   */
  public long getSize();
  
  /**
   * @return Raw file content
   */
  public byte[] getContent() throws IOException;
}
