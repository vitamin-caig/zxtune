/**
 * @file
 * @brief Base Vfs object interface
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.net.Uri;
import android.support.annotation.Nullable;

public interface VfsObject {

  /**
   * @return Public unique identifier
   */
  Uri getUri();

  /**
   * @return Display name
   */
  String getName();

  /**
   * @return Optional description
   */
  String getDescription();

  /**
   * Retrieve parent object or null
   */
  @Nullable
  VfsObject getParent();

  /**
   * Retrieve extension by specified ID
   *
   * @return null if not supported
   */
  @Nullable
  Object getExtension(String id);
}
