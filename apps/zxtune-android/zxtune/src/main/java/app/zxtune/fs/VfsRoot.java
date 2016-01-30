/**
 *
 * @file
 *
 * @brief Vfs root directory interface
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs;

import java.io.IOException;

import android.net.Uri;

interface VfsRoot extends VfsDir {

  /**
   * Tries to resolve object by uri
   * @param uri Identifier to resolve
   * @return Found object or null othervise
   */
  public VfsObject resolve(Uri uri) throws IOException;
}
