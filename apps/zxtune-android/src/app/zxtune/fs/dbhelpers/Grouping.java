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

import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.database.sqlite.SQLiteStatement;

public class Grouping {
  
  private static enum Fields {
    _id
  }
  
  private final String tableName;
  private final int bitsForObject;
  private final SQLiteStatement insertStatement;

  public static String createQuery(String tableName) {
    return String.format(Locale.US, "CREATE TABLE %s (%s INTEGER PRIMARY KEY);", tableName, Fields._id.name());
  }
  
  public Grouping(SQLiteOpenHelper helper, String tableName, int bitsForObject) {
    this.tableName = tableName;
    this.bitsForObject = bitsForObject;
    final String statement = String.format(Locale.US, "REPLACE INTO %s VALUES (?);", tableName); 
    this.insertStatement = helper.getWritableDatabase().compileStatement(statement);
  }
  
  public synchronized final void add(long group, long object) {
    final long id = getId(group, object);
    insertStatement.bindLong(1 + Fields._id.ordinal(), id);
    insertStatement.executeInsert();
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
