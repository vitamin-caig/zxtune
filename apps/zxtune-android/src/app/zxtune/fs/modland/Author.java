/**
 *
 * @file
 *
 * @brief Author description POJO
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.modland;

public final class Author {

  public final int id;
  public final String nickname;
  public final int tracks;

  public Author(int id, String nickname, int tracks) {
    this.id = id;
    this.nickname = nickname;
    this.tracks = tracks;
  }
}
