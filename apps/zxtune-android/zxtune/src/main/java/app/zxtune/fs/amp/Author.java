/**
 *
 * @file
 *
 * @brief Author description POJO
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.amp;

public final class Author {

  public final int id;
  public final String handle;
  public final String realName;
  //TODO: pictures

  public Author(int id, String handle, String realName) {
    this.id = id;
    this.handle = handle;
    this.realName = realName.equals("n/a") ? "" : realName;
  }
}
