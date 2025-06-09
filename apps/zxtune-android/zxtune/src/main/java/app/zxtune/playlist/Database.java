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
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteOpenHelper;
import android.util.SparseIntArray;

import androidx.annotation.Nullable;

import java.util.Locale;

import app.zxtune.Log;
import app.zxtune.fs.dbhelpers.DBStatistics;
import app.zxtune.fs.dbhelpers.Utils;

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
      enum Fields {
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
    
    static final class Statistics {
      enum Fields {
        count, locations, duration
      }

      private static final String[] COLUMNS = {
          "COUNT(*)",
          "COUNT(DISTINCT(location))",
          "SUM(duration)"
      };
    }

    static final class Positions {
      enum Fields {
        pos, track_id
      }

      private static final String NAME = "positions";
      private static final String CREATE_QUERY = "CREATE TABLE positions (" +
          "pos INTEGER PRIMARY KEY AUTOINCREMENT, " +
          "track_id INTEGER UNIQUE NOT NULL" +
          ");";
    }

    public static final class Playlist {
      public enum Fields {
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
    DBStatistics.send(dbHelper);
  }
  
  // ! @return Cursor with statistics
  final Cursor queryStatistics(@Nullable String selection) {
    Log.d(TAG, "queryStatistics(%s) called", selection);
    final SQLiteDatabase db = dbHelper.getReadableDatabase();
    return db.query(Tables.Tracks.NAME, Tables.Statistics.COLUMNS, selection, null/*selectionArgs*/, null/*groupBy*/,
        null/*having*/, null/*orderBy*/); 
  }

  // ! @return Cursor with queried values
  final Cursor queryPlaylistItems(@Nullable String[] columns, @Nullable String selection,
                                  @Nullable String[] selectionArgs, @Nullable String orderBy) {
    Log.d(TAG, "queryPlaylistItems(%s) called", selection);
    final SQLiteDatabase db = dbHelper.getReadableDatabase();
    return db.query(Tables.Playlist.NAME, columns, selection, selectionArgs, null/* groupBy */,
        null/* having */, orderBy);
  }
  
  // ! @return new item id
  final long insertPlaylistItem(@Nullable ContentValues values) {
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    return db.insert(Tables.Playlist.NAME, null, values);
  }

  // ! @return count of deleted items
  final int deletePlaylistItems(@Nullable String selection, @Nullable String[] selectionArgs) {
    Log.d(TAG, "deletePlaylistItems(%s) called", selection);
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    return db.delete(Tables.Playlist.NAME, selection, selectionArgs);
  }

  // ! @return count of updated items
  final int updatePlaylistItems(@Nullable ContentValues values, @Nullable String selection,
                                @Nullable String[] selectionArgs) {
    //update tracks table directly to avoid on update trigger
    Log.d(TAG, "updatePlaylistItems(%s) called", selection);
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    return db.update(Tables.Tracks.NAME, values, selection, selectionArgs);
  }
  
  /**
   * @param positions id => pos list
   */
  final void updatePlaylistItemsOrder(SparseIntArray positions) {
    //sqlite prior to 3.7.11 (api v14) does not support multiple values
    //http://stackoverflow.com/questions/2421189/version-of-sqlite-used-in-android
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    try {
      db.beginTransaction();
      for (int i = 0, lim = positions.size(); i != lim; ++i) {
        final String queryString = String.format(Locale.US, "REPLACE INTO %s(%s, %s) VALUES(%d, %d);",
            Tables.Positions.NAME, Tables.Positions.Fields.track_id, Tables.Positions.Fields.pos,
            positions.keyAt(i), positions.valueAt(i));
        db.execSQL(queryString);
      }
      db.setTransactionSuccessful();
    } finally {
      db.endTransaction();
    }
  }
  
  final void sortPlaylistItems(Tables.Playlist.Fields field, String order) {
    Log.d(TAG, "sortPlaylistItems(%s, %s) called", field.name(), order);
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    try {
      db.beginTransaction();
      db.delete(Tables.Positions.NAME, null, null);
      final String insertString = "INSERT INTO " + Tables.Positions.NAME + "(" + Tables.Positions.Fields.track_id + ")";
      final String selectString = "SELECT " + Tables.Tracks.Fields._id + " FROM " + Tables.Tracks.NAME + " ORDER BY " + field.name() + " " + order;
      db.execSQL(insertString + " " + selectString + ";"); 
      db.setTransactionSuccessful();
    } finally {
      db.endTransaction();
    }
  }
  
  private static class DBHelper extends SQLiteOpenHelper {

    DBHelper(Context context) {
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
      Log.d(TAG, "Upgrading database %d -> %d", oldVersion, newVersion);
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
        Log.w(TAG, e, "upgradeFromVer1");
      } finally {
        db.endTransaction();
      }
      db.execSQL("DROP TABLE IF EXISTS playlist;");
      onCreate(db);
    }
  }
}
