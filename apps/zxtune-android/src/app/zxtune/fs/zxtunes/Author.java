/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs.zxtunes;

/**
 *  Author descripting POJO 
 */
public final class Author {

  public final int id;
  public final String nickname;
  public final String name;

  public Author(int id, String nickname, String name) {
    this.id = id;
    this.nickname = nickname;
    this.name = name != null ? name : "".intern();
  }
}
