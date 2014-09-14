/**
 *
 * @file
 *
 * @brief Party description POJO
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.zxart;

public final class Party {

  public final int id;
  public final String name;
  public final int year;

  public Party(int id, String name, int year) {
    this.id = id;
    this.name = name;
    this.year = year;
  }
}
