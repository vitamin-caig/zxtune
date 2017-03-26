/**
 * @file
 * @brief Author description POJO
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modarchive;

public final class Author {

  public final int id;
  public final String alias;
  //TODO: pictures

  public Author(int id, String alias) {
    this.id = id;
    this.alias = alias;
  }

  //Currently API does not provide information about track without author info.
  //The only way to get them - perform search
  public static final Author UNKNOWN = new Author(0, "!Unknown");
}
