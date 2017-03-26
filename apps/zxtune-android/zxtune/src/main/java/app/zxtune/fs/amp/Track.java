/**
 * @file
 * @brief Track description POJO
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.amp;

public final class Track {

  /// unique id
  public final int id;
  /// display name
  public final String filename;
  /// in kb
  public final int size;

  public Track(int id, String filename, int size) {
    this.id = id;
    this.filename = filename;
    this.size = size;
  }
}
