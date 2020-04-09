package app.zxtune.fs.provider;

import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.NonNull;
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
  // int
  private static final String COLUMN_ICON = "icon";
  // string - size for file, not null if dir has feed
  private static final String COLUMN_DETAILS = "details";

  // int
  private static final String COLUMN_DONE = "done";
  // int
  private static final String COLUMN_TOTAL = "total";
  // string
  private static final String COLUMN_ERROR = "error";

  private static final int TYPE_DIR = 0;
  private static final int TYPE_FILE = 1;

  static class Listing {
    static final String[] COLUMNS = {COLUMN_TYPE, COLUMN_URI, COLUMN_NAME, COLUMN_DESCRIPTION
        , COLUMN_ICON, COLUMN_DETAILS};

    static Object[] makeDirectory(Uri uri, String name, String description,
                                  int icon, boolean hasFeed) {
      return new Object[]{TYPE_DIR, uri.toString(), name, description, icon, hasFeed ? "" : null};
    }

    static Object[] makeFile(Uri uri, String name, String description, String size) {
      return new Object[]{TYPE_FILE, uri.toString(), name, description, 0, size};
    }

    static Object[] makeLimiter() {
      return new Object[COLUMNS.length];
    }

    // For searches
    static boolean isLimiter(@NonNull Cursor cursor) {
      return cursor.isNull(1);
    }

    static boolean isDir(@NonNull Cursor cursor) {
      return cursor.getInt(0) == TYPE_DIR;
    }

    static Uri getUri(@NonNull Cursor cursor) {
      return Uri.parse(cursor.getString(1));
    }

    static String getName(@NonNull Cursor cursor) {
      return cursor.getString(2);
    }

    static String getDescription(@NonNull Cursor cursor) {
      return cursor.getString(3);
    }

    static int getIcon(@NonNull Cursor cursor) {
      return cursor.getInt(4);
    }

    static String getSize(@NonNull Cursor cursor) {
      return cursor.getString(5);
    }

    static boolean hasFeed(@NonNull Cursor cursor) {
      return !cursor.isNull(5);
    }
  }

  static class Parents {

    static final String[] COLUMNS = {COLUMN_URI, COLUMN_NAME, COLUMN_ICON};

    static Object[] make(Uri uri, String name, int icon) {
      return new Object[]{uri.toString(), name, icon};
    }

    static Uri getUri(@NonNull Cursor cursor) {
      return Uri.parse(cursor.getString(0));
    }

    static String getName(@NonNull Cursor cursor) {
      return cursor.getString(1);
    }

    static int getIcon(@NonNull Cursor cursor) {
      return cursor.getInt(2);
    }
  }

  static class Status {
    static final String[] COLUMNS = {COLUMN_DONE, COLUMN_TOTAL, COLUMN_ERROR};

    static Object[] makeError(@NonNull Exception e) {
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

    static boolean isStatus(@NonNull Cursor cursor) {
      return Arrays.equals(cursor.getColumnNames(), COLUMNS);
    }

    static int getDone(@NonNull Cursor cursor) {
      return cursor.getInt(0);
    }

    static int getTotal(@NonNull Cursor cursor) {
      return cursor.getInt(1);
    }

    @Nullable
    static String getError(@NonNull Cursor cursor) {
      return cursor.getString(2);
    }
  }
}
