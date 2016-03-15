/**
 * 
 * @file
 *
 * @brief Stub object base for safe inheritance
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs;

abstract class StubObject implements VfsObject {

  @Override
  public String getDescription() {
    return "".intern();
  }

  @Override
  public Object getExtension(String id) {
    return null;
  }
}
