/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.dbhelpers;

import android.database.sqlite.SQLiteStatement;

import java.io.IOException;
import java.security.InvalidParameterException;

public class Objects {

  private final int fields;
  private final SQLiteStatement insertStatement;

  public Objects(DBProvider helper, String name, String mode, int fields) throws IOException {
    final String statement = makeInsertStatement(mode, name, fields);
    this.fields = fields;
    this.insertStatement = helper.getWritableDatabase().compileStatement(statement);
  }

  public Objects(DBProvider helper, String name, int fields) throws IOException {
    this(helper, name, "REPLACE", fields);
  }

  public final synchronized void add(Object... objects) {
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
  public final synchronized void add(long p1) {
    checkFieldsCount(1);
    bind(1, p1);
    insertStatement.executeInsert();
  }

  public final synchronized void add(String p1) {
    checkFieldsCount(1);
    bind(1, p1);
    insertStatement.executeInsert();
  }

  public final synchronized void add(long p1, String p2) {
    checkFieldsCount(2);
    insertStatement.clearBindings();
    bind(1, p1);
    bind(2, p2);
    insertStatement.executeInsert();
  }

  public final synchronized void add(String p1, long p2) {
    checkFieldsCount(2);
    insertStatement.clearBindings();
    bind(1, p1);
    bind(2, p2);
    insertStatement.executeInsert();
  }

  public final synchronized void add(String p1, String p2) {
    checkFieldsCount(2);
    insertStatement.clearBindings();
    bind(1, p1);
    bind(2, p2);
    insertStatement.executeInsert();
  }

  public final synchronized void add(long p1, String p2, String p3) {
    checkFieldsCount(3);
    insertStatement.clearBindings();
    bind(1, p1);
    bind(2, p2);
    bind(3, p3);
    insertStatement.executeInsert();
  }

  public final synchronized void add(long p1, String p2, long p3) {
    checkFieldsCount(3);
    insertStatement.clearBindings();
    bind(1, p1);
    bind(2, p2);
    bind(3, p3);
    insertStatement.executeInsert();
  }

  public final synchronized void add(String p1, String p2, long p3) {
    checkFieldsCount(3);
    insertStatement.clearBindings();
    bind(1, p1);
    bind(2, p2);
    bind(3, p3);
    insertStatement.executeInsert();
  }

  private void bind(int pos, String p) {
    if (p == null) {
      insertStatement.bindNull(pos);
    } else {
      insertStatement.bindString(pos, p);
    }
  }

  private void bind(int pos, long p) {
    insertStatement.bindLong(pos, p);
  }

  private void checkFieldsCount(int objectsCount) {
    if (objectsCount != fields) {
      throw new InvalidParameterException();
    }
  }

  private static String makeInsertStatement(String mode, String name, int fields) {
    final StringBuilder builder = new StringBuilder();
    builder.append(mode);
    builder.append(" INTO ");
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
