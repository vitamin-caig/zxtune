/**
 * @file
 * @brief Different db-related utils
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.dbhelpers;

import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import androidx.room.RoomDatabase;
import androidx.sqlite.db.SupportSQLiteDatabase;
import androidx.sqlite.db.SupportSQLiteOpenHelper;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Locale;

import app.zxtune.Log;
import app.zxtune.analytics.Analytics;

public final class Utils {

  private static final String TAG = Utils.class.getName();

  public static void cleanupDb(SQLiteDatabase db) {
    final String[] TYPES = {"table", "view", "index", "trigger"};
    for (String type : TYPES) {
      for (String name : getObjects(db::rawQuery, type)) {
        Log.d("CleanupDb", "Drop %s '%s'", type, name);
        db.execSQL(String.format(Locale.US, "DROP %S IF EXISTS %s;", type, name));
      }
    }
  }

  public static void sendStatistics(SQLiteOpenHelper helper) {
    try {
      final SQLiteDatabase db = helper.getReadableDatabase();
      sendStatistics(helper.getDatabaseName(), db.getPath(), db::rawQuery);
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to send DB statistics");
    }
  }

  public static void sendStatistics(SupportSQLiteOpenHelper helper) {
    try {
      final SupportSQLiteDatabase db = helper.getReadableDatabase();
      sendStatistics(helper.getDatabaseName(), db.getPath(), db::query);
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to send DB statistics");
    }
  }

  private static void sendStatistics(String dbName, String path, DbQuery db) {
    final long start = System.currentTimeMillis();
    final long size = new File(path).length();
    final HashMap<String, Long> tables = new HashMap<>();
    for (String table : getObjects(db, "table")) {
      tables.put(table, getRecordsCount(db, table));
    }
    Analytics.sendDbMetrics(dbName, size, tables, System.currentTimeMillis() - start);
  }

  private interface DbQuery {
    Cursor query(String query, String[] selectionArgs);
  }

  private static ArrayList<String> getObjects(DbQuery db, String type) {
    final String[] selectionArgs = {type};
    final Cursor cursor = db.query("SELECT name FROM sqlite_master WHERE type = ?", selectionArgs);
    final ArrayList<String> result = new ArrayList<>(cursor.getCount());
    try {
      while (cursor.moveToNext()) {
        final String name = cursor.getString(0);
        if (!"android_metadata".equals(name) && !"sqlite_sequence".equals(name) && !"room_master_table".equals(name)) {
          result.add(name);
        }
      }
    } finally {
      cursor.close();
    }
    return result;
  }

  private static long getRecordsCount(DbQuery db, String table) {
    final String[] selectionArgs = {};
    final Cursor cursor = db.query("SELECT COUNT(*) FROM " + table, selectionArgs);
    try {
      if (cursor.moveToFirst()) {
        return cursor.getLong(0);
      }
    } finally {
      cursor.close();
    }
    return -1;
  }

  public interface ThrowingRunnable {
    void run() throws IOException;
  }

  public static void runInTransaction(DBProvider helper, ThrowingRunnable cmd) throws IOException {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.beginTransaction();
    try {
      cmd.run();
      db.setTransactionSuccessful();
    } finally {
      db.endTransaction();
    }
  }

  @SuppressWarnings("RedundantThrows")
  public static void runInTransaction(RoomDatabase db, ThrowingRunnable cmd) throws IOException {
    db.runInTransaction(() -> run(cmd));
  }

  private static void run(ThrowingRunnable cmd) {
    try {
      cmd.run();
    } catch (IOException e) {
      sneakyThrow(e);
    }
  }

  @SuppressWarnings("unchecked")
  private static <E extends Throwable> void sneakyThrow(Throwable e) throws E {
    throw (E) e;
  }
}
