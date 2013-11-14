/**
 *
 * @file
 *
 * @brief Playlist DAL
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playlist;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.text.TextUtils;
import android.util.Log;

/*
 * Playlist DB model.
 * version 1
 * Tables; 'playlist' - all entries
 * fields:
 * '_id', integer primary autoincrement - unique entry identifier
 * 'location', text not null- source data identifier including subpath etc
 * 'type', text not null- chiptune type identifier
 * 'author', text - cached/modified entry's author
 * 'title', text - cached/modified entry's title
 * 'duration', integer not null- cached entry's duration in mS
 * 'tags', text - space separated list of hash prefixed words
 * 'properties', text - comma separated list of name=value entries
 */

class Database {

  private static final String NAME = "playlist";
  private static final int VERSION = 1;
  private static final String TAG = Database.class.getName();

  static final class Tables {
    static final class Playlist {
      static enum Fields {
        //use lowercased names due to restrictions for _id column name
        _id, location, type, author, title, duration, tags, properties
      }

      private static final String NAME = "playlist";
      private static final String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
          + " INTEGER PRIMARY KEY AUTOINCREMENT, " + Fields.location + " TEXT NOT NULL, "
          + Fields.type + " TEXT NOT NULL, " + Fields.author + " TEXT, " + Fields.title + " TEXT, "
          + Fields.duration + " INTEGER NOT NULL, " + Fields.tags + " text, " + Fields.properties
          + " text" + ");";
    }
  }

  private final DBHelper dbHelper;

  Database(Context context) {
    this.dbHelper = new DBHelper(context);
  }

  //! @return Cursor with queried values
  final Cursor queryPlaylistItems(String[] columns, String selection, String[] selectionArgs,
      String orderBy) {
    Log.d(TAG, "queryPlaylistItems(" + selection + ") called");
    final SQLiteDatabase db = dbHelper.getReadableDatabase();
    return db.query(Tables.Playlist.NAME, columns, selection, selectionArgs, null/* groupBy */,
        null/* having */, orderBy);
  }

  //! @return new item id
  final long insertPlaylistItem(ContentValues values) {
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    return db.insert(Tables.Playlist.NAME, null, values);
  }

  //! @return count of deleted items
  final int deletePlaylistItems(String selection, String[] selectionArgs) {
    Log.d(TAG, "deletePlaylistItems(" + selection + ") called");
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    return db.delete(Tables.Playlist.NAME, selection, selectionArgs);
  }

  //! @return count of updated items
  final int updatePlaylistItems(ContentValues values, String selection, String[] selectionArgs) {
    Log.d(TAG, "updatePlaylistItems(" + selection + ") called");
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    return db.update(Tables.Playlist.NAME, values, selection, selectionArgs);
  }

  static String defaultPlaylistOrder() {
    return Tables.Playlist.Fields._id + " ASC";
  }

  static String playlistSelection(String origSelection, Long id) {
    if (id != null) {
      final String subselection = Tables.Playlist.Fields._id + " = " + id;
      return TextUtils.isEmpty(origSelection) ? subselection : origSelection + " AND "
          + subselection;
    } else {
      return origSelection;
    }
  }

  private static class DBHelper extends SQLiteOpenHelper {

    public DBHelper(Context context) {
      super(context, NAME, null, VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
      Log.d(TAG, "Creating database");
      db.execSQL(Tables.Playlist.CREATE_QUERY);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
      Log.d(TAG, String.format("Upgrading database %u -> %u", oldVersion, newVersion));
    }
  }
}
