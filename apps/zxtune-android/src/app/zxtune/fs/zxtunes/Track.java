/**
 *
 * @file
 *
 * @brief Track description POJO
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.zxtunes;

public final class Track {

  public final int id;
  public final String filename;
  public final String title;
  public final Integer duration;
  public final Integer date;
  
  public Track(int id, String filename, String title, Integer duration, Integer date) {
    this.id = id;
    this.filename = filename;
    this.title = title;
    this.duration = duration;
    this.date = date;
  }
}
