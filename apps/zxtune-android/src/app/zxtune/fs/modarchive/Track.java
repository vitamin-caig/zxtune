/**
 *
 * @file
 *
 * @brief Track description POJO
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.modarchive;

public final class Track {

  public final int id;
  public final String filename;
  public final String title;
  public final int size;

  public Track(int id, String filename, String title, int size) {
    this.id = id;
    this.filename = filename;
    this.title = title;
    this.size = size;
  }
}
