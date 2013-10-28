/**
 * @file
 * @brief
 * @version $Id:$
 * @author
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
}
