/**
 *
 * @file
 *
 * @brief Group description POJO
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.modland;

public final class Group {

  public final int id;
  public final String name;
  public final int tracks;

  public Group(int id, String name, int tracks) {
    this.id = id;
    this.name = name;
    this.tracks = tracks;
  }
}
