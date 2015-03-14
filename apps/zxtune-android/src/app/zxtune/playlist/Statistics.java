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

import java.util.concurrent.TimeUnit;

import android.database.Cursor;
import app.zxtune.TimeStamp;

public class Statistics {

  final private int items;
  final private int locations;
  final private TimeStamp duration;

  public Statistics(Cursor cursor) {
    items = cursor.getInt(Database.Tables.Statistics.Fields.count.ordinal());
    locations = cursor.getInt(Database.Tables.Statistics.Fields.locations.ordinal());
    duration = TimeStamp.createFrom(cursor.getInt(Database.Tables.Statistics.Fields.duration.ordinal()), TimeUnit.MILLISECONDS);
  }
  
  final public int getTotal() {
    return items;
  }
  
  final public int getLocations() {
    return locations;
  }

  final public TimeStamp getDuration() {
    return duration;
  }
}
