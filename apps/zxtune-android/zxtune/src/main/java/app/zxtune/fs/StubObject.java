/**
 * @file
 * @brief Stub object base for safe inheritance
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.support.annotation.Nullable;

abstract class StubObject implements VfsObject {

  @Override
  public String getDescription() {
    return "";
  }

  @Override
  @Nullable
  public Object getExtension(String id) {
    return null;
  }
}
