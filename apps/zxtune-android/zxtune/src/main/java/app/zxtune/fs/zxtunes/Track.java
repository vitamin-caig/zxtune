/**
 * @file
 * @brief Track description POJO
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxtunes;

import androidx.annotation.Nullable;

public final class Track {

  public final int id;
  public final String filename;
  public final String title;
  @Nullable
  public final Integer duration;
  @Nullable
  public final Integer date;

  public Track(int id, String filename, String title, @Nullable Integer duration, @Nullable Integer date) {
    this.id = id;
    this.filename = filename;
    this.title = title;
    this.duration = duration;
    this.date = date;
  }
}
