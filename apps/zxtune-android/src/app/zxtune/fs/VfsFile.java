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
   * @return File size in implementation-specific units
   */
  public String getSize();
  
  /**
   * @return Raw file content
   */
  public byte[] getContent() throws IOException;
}
