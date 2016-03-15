/**
 *
 * @file
 *
 * @brief Track description POJO
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.zxart;

public final class Track {

  public final int id;
  public final String filename;
  public final String title;
  public final String votes;
  public final String duration;
  public final int year;
  public final String compo;
  public final int partyplace;
  
  public Track(int id, String filename, String title, String votes, String duration, int year, String compo, int partyplace) {
    this.id = id;
    this.filename = filename;
    this.title = title;
    this.votes = votes;
    this.duration = duration;
    this.year = year;
    this.compo = compo;
    this.partyplace = partyplace;
  }
}
