/**
 * 
 * @file
 *
 * @brief Base class for objects grouping
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs.dbhelpers;

import java.util.Locale;

import android.content.ContentValues;
import android.database.sqlite.SQLiteQueryBuilder;

public class Grouping {
  
  private static enum Fields {
    _id
  }
  
  private final String tableName;
  private final int bitsForObject;
  
  public Grouping(String tableName, int bitsForObject) {
    this.tableName = tableName;
    this.bitsForObject = bitsForObject;
  }
  
  public final String getTableName() {
    return tableName;
  }
  
  public final String createQuery() {
    return String.format(Locale.US, "CREATE TABLE %s (%s INTEGER PRIMARY KEY);", tableName, Fields._id.name());
  }
  
  public final ContentValues createValues(long group, long object) {
    final ContentValues res = new ContentValues();
    res.put(Fields._id.name(), getId(group, object));
    return res;
  }

  public final String getIdsSelection(long group) {
    final long mask = getId(1, 0) - 1;
    final long lower = getId(group, 0);
    final String groupSelection = String.format(Locale.US, "%s BETWEEN %d AND %d", Fields._id.name(), lower, lower | mask); 
    final String objectSelection = String.format(Locale.US, "%s & %d", Fields._id.name(), mask);
    return SQLiteQueryBuilder.buildQueryString(true, tableName,
          new String[]{objectSelection}, groupSelection, null, null, null, null);
  }

  private long getId(long group, long object) {
    return (group << bitsForObject) | object;
  }
}
