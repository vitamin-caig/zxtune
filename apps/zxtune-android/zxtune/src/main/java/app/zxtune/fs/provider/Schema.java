package app.zxtune.fs.provider;

import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.Arrays;

class Schema {
  // int
  private static final String COLUMN_TYPE = "type";
  // uri
  private static final String COLUMN_URI = "uri";
  // string
  private static final String COLUMN_NAME = "name";
  // string
  private static final String COLUMN_DESCRIPTION = "description";
  // int - icon res or tracks count
  private static final String COLUMN_ICON = "icon";
  // string - size for file, not null if dir has feed
  private static final String COLUMN_DETAILS = "details";
  // bool
  private static final String COLUMN_CACHED = "cached";

  // int
  private static final String COLUMN_DONE = "done";
  // int
  private static final String COLUMN_TOTAL = "total";
  // string
  private static final String COLUMN_ERROR = "error";

  private static final int TYPE_DIR = 0;
  private static final int TYPE_FILE = 1;

  @Nullable
  private static Integer getIntOrNull(Cursor cursor, int column) {
    return cursor.isNull(column) ? null : cursor.getInt(column);
  }

  static class Listing {
    static final String[] COLUMNS = {COLUMN_TYPE, COLUMN_URI, COLUMN_NAME, COLUMN_DESCRIPTION
        , COLUMN_ICON, COLUMN_DETAILS, COLUMN_CACHED};

    static Object[] makeDirectory(Uri uri, String name, String description,
                                  @Nullable Integer icon, boolean hasFeed) {
      return new Object[]{TYPE_DIR, uri.toString(), name, description, icon, hasFeed ? "" : null,
          null};
    }

    static Object[] makeFile(Uri uri, String name, String description, String details,
                             @Nullable Integer tracks, @Nullable Boolean isCached) {
      return new Object[]{TYPE_FILE, uri.toString(), name, description, tracks, details,
          isCached != null ? (isCached ? 1 : 0) : null};
    }

    static Object[] makeLimiter() {
      return new Object[COLUMNS.length];
    }

    // For searches
    static boolean isLimiter(Cursor cursor) {
      return cursor.isNull(1);
    }

    static boolean isDir(Cursor cursor) {
      return cursor.getInt(0) == TYPE_DIR;
    }

    static Uri getUri(Cursor cursor) {
      return Uri.parse(cursor.getString(1));
    }

    static String getName(Cursor cursor) {
      return cursor.getString(2);
    }

    static String getDescription(Cursor cursor) {
      return cursor.getString(3);
    }

    @Nullable
    static Integer getIcon(Cursor cursor) {
      return getIntOrNull(cursor, 4);
    }

    static String getDetails(Cursor cursor) {
      return cursor.getString(5);
    }

    static boolean hasFeed(Cursor cursor) {
      return !cursor.isNull(5);
    }

    @Nullable
    static Integer getTracks(Cursor cursor) {
      return getIntOrNull(cursor, 4);
    }

    @Nullable
    static Boolean isCached(Cursor cursor) {
      return cursor.isNull(6) ? null : (cursor.getInt(6) != 0);
    }
  }

  static class Parents {

    static final String[] COLUMNS = {COLUMN_URI, COLUMN_NAME, COLUMN_ICON};

    static Object[] make(Uri uri, String name, @Nullable Integer icon) {
      return new Object[]{uri.toString(), name, icon};
    }

    static Uri getUri(Cursor cursor) {
      return Uri.parse(cursor.getString(0));
    }

    static String getName(Cursor cursor) {
      return cursor.getString(1);
    }

    @Nullable
    static Integer getIcon(Cursor cursor) {
      return getIntOrNull(cursor, 2);
    }
  }

  static class Status {
    static final String[] COLUMNS = {COLUMN_DONE, COLUMN_TOTAL, COLUMN_ERROR};

    static Object[] makeError(Exception e) {
      final Throwable cause = e.getCause();
      final String msg = cause != null ? cause.getMessage() : e.getMessage();
      return new Object[]{0, 0, msg};
    }

    static Object[] makeIntermediateProgress() {
      return makeProgress(-1);
    }

    static Object[] makeProgress(int done) {
      return makeProgress(done, 100);
    }

    static Object[] makeProgress(int done, int total) {
      return new Object[]{done, total, null};
    }

    static boolean isStatus(Cursor cursor) {
      return Arrays.equals(cursor.getColumnNames(), COLUMNS);
    }

    static int getDone(Cursor cursor) {
      return cursor.getInt(0);
    }

    static int getTotal(Cursor cursor) {
      return cursor.getInt(1);
    }

    @Nullable
    static String getError(Cursor cursor) {
      return cursor.getString(2);
    }
  }
}
