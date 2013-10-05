/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs.zxtunes;

/**
 *  Track description POJO
 */
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
