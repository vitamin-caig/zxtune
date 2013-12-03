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

import java.util.Arrays;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.Log;
import android.util.SparseIntArray;

/*
 * Playlist DB model.
 */

/*
 * Version 1
 */

/*
 * Table 'playlist' - all entries
 * '_id', integer primary autoincrement - unique entry identifier
 * 'location', text not null- source data identifier including subpath etc
 * 'type', text not null- chiptune type identifier
 * 'author', text - cached/modified entry's author
 * 'title', text - cached/modified entry's title
 * 'duration', integer not null- cached entry's duration in mS
 * 'tags', text - space separated list of hash prefixed words
 * 'properties', text - comma separated list of name=value entries
 */

/*
 * Version 2
 * Use two tables and aggregating view to have two autoincrement columns and easy order changing 
 * (insert or replace instead of bunch of updates).
 */

/*
 * Table 'tracks' - metadata
 * '_id', integer primary autoincrement - unique entry identifier
 * 'location', text not null - source data identifier including subpath etc
 * 'author', text - cached/modified entry's author
 * 'title', text - cached/modified entry's title
 * 'duration', integer not null- cached entry's duration in mS
 * 'properties', blob - encoded properties
 */

/*
 * Table 'positions' - relative positions
 * 'pos', integer primary autoincrement - unique relative position
 * 'track_id', integer not null - identifier of track
 */

/*
 * Table 'playlist' - virtual table
 * _id, pos, location, author, title, duration, properties
 * playlist_insert, playlist_delete - triggers
 */

public class Database {

  private static final String NAME = "playlist";
  private static final int VERSION = 2;
  private static final String TAG = Database.class.getName();

  public static final class Tables {
    static final class Tracks {
      static enum Fields {
        // use lower cased names due to restrictions for _id column name
        _id, location, author, title, duration, properties
      }

      private static final String NAME = "tracks";
      private static final String CREATE_QUERY = "CREATE TABLE tracks (" +
          "_id INTEGER PRIMARY KEY AUTOINCREMENT, " +
          "location TEXT NOT NULL, " +
          "author TEXT, " +
          "title TEXT, " +
          "duration INTEGER NOT NULL, " +
          "properties BLOB" +
          ");";
    }

    static final class Positions {
      static enum Fields {
        pos, track_id
      }

      private static final String NAME = "positions";
      private static final String CREATE_QUERY = "CREATE TABLE positions (" +
          "pos INTEGER PRIMARY KEY AUTOINCREMENT, " +
          "track_id INTEGER UNIQUE NOT NULL" +
          ");";
    }

    public static final class Playlist {
      public static enum Fields {
        _id, pos, location, author, title, duration, properties
      }

      static final String NAME = "playlist";
      private static final String CREATE_QUERY =
          "CREATE VIEW playlist AS " +
              "SELECT _id, pos, location, author, title, duration, properties FROM positions " +
              "LEFT OUTER JOIN tracks ON positions.track_id=tracks._id ORDER BY pos ASC;";
      private static final String CREATE_INSERT_TRIGGER_QUERY =
          "CREATE TRIGGER playlist_insert INSTEAD OF INSERT ON playlist " +
              "FOR EACH ROW " +
              "BEGIN " +
              "INSERT INTO tracks(location, author, title, duration, properties)" +
              " VALUES(new.location, new.author, new.title, new.duration, new.properties);" +
              "INSERT INTO positions(track_id) SELECT MAX(_id) FROM tracks;" +
              "END;";
      private static final String CREATE_DELETE_TRIGGER_QUERY =
          "CREATE TRIGGER playlist_delete INSTEAD OF DELETE ON playlist " +
              "FOR EACH ROW " +
              "BEGIN " +
              "DELETE FROM tracks WHERE _id=old._id;" +
              "DELETE FROM positions WHERE pos=old.pos;" +
              "END;";
    }
  }

  private final DBHelper dbHelper;

  public Database(Context context) {
    this.dbHelper = new DBHelper(context);
  }

  // ! @return Cursor with queried values
  public final Cursor queryPlaylistItems(String[] columns, String selection, String[] selectionArgs,
      String orderBy) {
    Log.d(TAG, "queryPlaylistItems(" + selection + ") called");
    final SQLiteDatabase db = dbHelper.getReadableDatabase();
    return db.query(Tables.Playlist.NAME, columns, selection, selectionArgs, null/* groupBy */,
        null/* having */, orderBy);
  }
  
  // ! @return new item id
  public final long insertPlaylistItem(ContentValues values) {
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    return db.insert(Tables.Playlist.NAME, null, values);
  }

  // ! @return count of deleted items
  public final int deletePlaylistItems(String selection, String[] selectionArgs) {
    Log.d(TAG, "deletePlaylistItems(" + selection + ") called");
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    return db.delete(Tables.Playlist.NAME, selection, selectionArgs);
  }

  // ! @return count of updated items
  public final int updatePlaylistItems(ContentValues values, String selection, String[] selectionArgs) {
    //update tracks table directly to avoid on update trigger
    Log.d(TAG, "updatePlaylistItems(" + selection + ") called");
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    return db.update(Tables.Tracks.NAME, values, selection, selectionArgs);
  }
  
  /*
   * @param ids id => pos list
   */
  public final void updatePlaylistItemsOrder(SparseIntArray positions) {
    final String[] pair = {Tables.Positions.Fields.track_id.name(), Tables.Positions.Fields.pos.name()};
    final StringBuilder query = new StringBuilder();
    query.append("REPLACE INTO ");
    query.append(Tables.Positions.NAME);
    query.append(toString(pair));
    query.append(" VALUES");
    for (int i = 0, lim = positions.size(); i != lim; ++i) {
      if (i != 0) {
        query.append(',');
      }
      pair[0] = Integer.toString(positions.keyAt(i));
      pair[1] = Integer.toString(positions.valueAt(i));
      query.append(toString(pair));
    }
    query.append(';');
    final String queryString = query.toString();
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    db.execSQL(queryString);
  }
  
  private static String toString(String[] array) {
    final String res = Arrays.toString(array);
    return res.replace('[', '(').replace(']', ')');
  }

  private static class DBHelper extends SQLiteOpenHelper {

    public DBHelper(Context context) {
      super(context, NAME, null, VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
      Log.d(TAG, "Creating database");
      db.execSQL(Tables.Tracks.CREATE_QUERY);
      db.execSQL(Tables.Positions.CREATE_QUERY);
      db.execSQL(Tables.Playlist.CREATE_QUERY);
      db.execSQL(Tables.Playlist.CREATE_INSERT_TRIGGER_QUERY);
      db.execSQL(Tables.Playlist.CREATE_DELETE_TRIGGER_QUERY);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
      Log.d(TAG, String.format("Upgrading database %d -> %d", oldVersion, newVersion));
      if (oldVersion == 1) {
        upgradeFromVer1(db);
      }
    }

    private void upgradeFromVer1(SQLiteDatabase db) {
      try {
        db.beginTransaction();
        db.execSQL("ALTER TABLE playlist RENAME TO playlist_temp");
        onCreate(db);
        db.execSQL("INSERT INTO playlist(_id, location, author, title, duration) " +
            "SELECT _id, location, author, title, duration FROM playlist_temp;");
        db.execSQL("DROP TABLE playlist_temp");
        db.setTransactionSuccessful();
        return;
      } catch (SQLiteException e) {
        Log.d(TAG, "upgradeFromVer1", e);
      } finally {
        db.endTransaction();
      }
      db.execSQL("DROP TABLE IF EXISTS playlist;");
      onCreate(db);
    }
  }
}
