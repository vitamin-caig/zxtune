/**
 *
 * @file
 *
 * @brief Base Vfs object interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs;

import android.net.Uri;

public interface VfsObject {

  /**
   * @return Public unique identifier
   */
  public Uri getUri();

  /**
   * @return Display name
   */
  public String getName();
  
  /**
   * @return Optional description
   */
  public String getDescription();

  /**
   * Retrieve parent object or null
   */
  public VfsObject getParent();
  
  /**
   * Retrieve extension by specified ID
   * @return null if not supported
   */
  public Object getExtension(String id);
}
