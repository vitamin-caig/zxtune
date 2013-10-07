/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs;

import java.io.IOException;

public interface VfsDir extends VfsObject {

  /**
   * Directory content enumerating callback
   */
  public interface Visitor {
    
    public enum Status {
      STOP,
      CONTINUE
    }
    
    /**
     * Called on visited directory
     */
    public Status onDir(VfsDir dir);

    /**
     * Called on visited file
     */
    public Status onFile(VfsFile file);
  }
  
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
