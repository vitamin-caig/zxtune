/**
 *
 * @file
 *
 * @brief Database (metadata) item
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playlist;

import android.database.Cursor;

import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;

public class Statistics {

  private final int items;
  private final int locations;
  private final TimeStamp duration;

  public Statistics(Cursor cursor) {
    items = cursor.getInt(Database.Tables.Statistics.Fields.count.ordinal());
    locations = cursor.getInt(Database.Tables.Statistics.Fields.locations.ordinal());
    duration = TimeStamp.createFrom(cursor.getInt(Database.Tables.Statistics.Fields.duration.ordinal()), TimeUnit.MILLISECONDS);
  }
  
  public final int getTotal() {
    return items;
  }
  
  public final int getLocations() {
    return locations;
  }

  public final TimeStamp getDuration() {
    return duration;
  }
}
