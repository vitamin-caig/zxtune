/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs.dbhelpers;

import java.security.InvalidParameterException;

import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteStatement;

public class Objects {
  
  private final int fields;
  private final SQLiteStatement insertStatement;

  public Objects(SQLiteOpenHelper helper, String name, int fields) {
    final String statement = makeInsertStatement(name, fields);
    this.fields = fields;
    this.insertStatement = helper.getWritableDatabase().compileStatement(statement);
  }
  
  public synchronized final void add(Object... objects) {
    checkFieldsCount(objects.length);
    insertStatement.clearBindings();
    for (int idx = 0; idx != objects.length; ++idx) {
      final Object obj = objects[idx];
      if (obj == null) {
        insertStatement.bindNull(1 + idx);
      } else if (obj instanceof String) {
        insertStatement.bindString(1 + idx, (String) obj);
      } else if (obj instanceof Long) {
        insertStatement.bindLong(1 + idx, (Long) obj);
      } else {
        insertStatement.bindLong(1 + idx, (Integer) obj);
      }
    }
    insertStatement.executeInsert();
  }
  
  //particular cases to avoid temporary arrays and type casts
  public synchronized final void add(long p1) {
    checkFieldsCount(1);
    insertStatement.bindLong(1, p1);
    insertStatement.executeInsert();
  }

  public synchronized final void add(String p1) {
    checkFieldsCount(1);
    insertStatement.bindString(1, p1);
    insertStatement.executeInsert();
  }
  
  public synchronized final void add(long p1, String p2) {
    checkFieldsCount(2);
    insertStatement.clearBindings();
    insertStatement.bindLong(1, p1);
    if (p2 == null) {
      insertStatement.bindNull(2);
    } else {
      insertStatement.bindString(2, p2);
    }
    insertStatement.executeInsert();
  }
  
  public synchronized final void add(long p1, String p2, String p3) {
    checkFieldsCount(3);
    insertStatement.clearBindings();
    insertStatement.bindLong(1, p1);
    if (p2 == null) {
      insertStatement.bindNull(2);
    } else {
      insertStatement.bindString(2, p2);
    }
    if (p3 == null) {
      insertStatement.bindNull(3);
    } else {
      insertStatement.bindString(3, p3);
    }
    insertStatement.executeInsert();
  }

  public synchronized final void add(long p1, String p2, long p3) {
    checkFieldsCount(3);
    insertStatement.clearBindings();
    insertStatement.bindLong(1, p1);
    if (p2 == null) {
      insertStatement.bindNull(2);
    } else {
      insertStatement.bindString(2, p2);
    }
    insertStatement.bindLong(3, p3);
    insertStatement.executeInsert();
  }
  
  private void checkFieldsCount(int objectsCount) {
    if (objectsCount != fields) {
      throw new InvalidParameterException();
    }
  }
  
  private static String makeInsertStatement(String name, int fields) {
    final StringBuilder builder = new StringBuilder();
    builder.append("REPLACE INTO ");
    builder.append(name);
    builder.append(" VALUES (");
    for (int pos = 0; pos < fields; ++pos) {
      if (pos != 0) {
        builder.append(", ");
      }
      builder.append('?');
    }
    builder.append(");");
    return builder.toString();
  }
}
