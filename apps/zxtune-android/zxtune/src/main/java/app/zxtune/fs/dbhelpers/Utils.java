/**
 * @file
 * @brief Different db-related utils
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.dbhelpers;

import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;

import java.util.ArrayList;
import java.util.Locale;

import app.zxtune.Log;

public final class Utils {

  public static void cleanupDb(SQLiteDatabase db) {
    final String TYPES[] = {"table", "view", "index", "trigger"};
    for (String type : TYPES) {
      for (String name : getObjects(db, type)) {
        Log.d("CleanupDb", "Drop %s '%s'", type, name);
        db.execSQL(String.format(Locale.US, "DROP %S IF EXISTS %s;", type, name));
      }
    }
  }

  private static ArrayList<String> getObjects(SQLiteDatabase db, String type) {
    final String[] columns = {"name"};
    final String selection = "type = ?";
    final String[] selectionArgs = {type};
    final Cursor cursor = db.query("sqlite_master", columns, selection, selectionArgs, null, null, null);
    final ArrayList<String> result = new ArrayList<String>(cursor.getCount());
    try {
      while (cursor.moveToNext()) {
        final String name = cursor.getString(0);
        if (!name.equals("android_metadata")) {
          result.add(name);
        }
      }
    } finally {
      cursor.close();
    }
    return result;
  }
}
