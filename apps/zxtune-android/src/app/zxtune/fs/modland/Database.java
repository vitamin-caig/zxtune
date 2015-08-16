/**
 *
 * @file
 *
 * @brief DAL helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.modland;

import java.util.HashMap;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.Grouping;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;
import app.zxtune.fs.dbhelpers.Utils;

/**
 * Version 1
 *
 * CREATE TABLE {authors,collections,formats} (_id INTEGER PRIMARY KEY, name TEXT NOT NULL, tracks INTEGER)
 * CREATE TABLE tracks (_id INTEGER PRIMARY KEY, path TEXT NOT NULL, size INTEGER)
 * CREATE TABLE {authors,collections,formats}_tracks (hash INTEGER UNIQUE, group_id INTEGER, track_id INTEGER)
 *
 * use hash as 10000000000 * group_id + track_id to support multiple insertions of same pair
 * 
 * 
 * Version 2
 * 
 * Use standard grouping for {authors,collections,formats}_tracks
 * Use timestamps
 * Store decoded paths
 */

final class Database {

  final static String TAG = Database.class.getName();

  final static String NAME = "modland.com";
  final static int VERSION = 2;

  final static class Tables {

    final static class Groups {

      static enum Fields {
        _id, name, tracks
      }

      static final String getCreateQuery(String name) {
        return "CREATE TABLE " + name + " (" + Fields._id
              + " INTEGER PRIMARY KEY, " + Fields.name + " TEXT NOT NULL, " + Fields.tracks
              + " INTEGER);";
      }

      static Group createGroup(Cursor cursor) {
        final int id = cursor.getInt(Fields._id.ordinal());
        final String title = cursor.getString(Fields.name.ordinal());
        final int tracks = cursor.getInt(Fields.tracks.ordinal());
        return new Group(id, title, tracks);
      }

      static ContentValues createValues(Group obj) {
        final ContentValues res = new ContentValues();
        res.put(Fields._id.name(), obj.id);
        res.put(Fields.name.name(), obj.name);
        res.put(Fields.tracks.name(), obj.tracks);
        return res;
      }
    }

    final static class Authors {

      static final String NAME = "authors";
    }

    final static class Collections {

      static final String NAME = "collections";
    }

    final static class Formats {

      static final String NAME = "formats";
    }
    
    final static String LIST[] = {Authors.NAME, Collections.NAME, Formats.NAME};
    
    final static class Tracks {

      static enum Fields {
        _id, path, size
      }

      final static String NAME = "tracks";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
              + " INTEGER PRIMARY KEY, " + Fields.path + " TEXT NOT NULL, " + Fields.size + " INTEGER);";
      
      static String getSelection(String subquery) {
        return Fields._id + " IN (" + subquery + ")";
      }

      static Track createTrack(Cursor cursor) {
        final int id = cursor.getInt(Fields._id.ordinal());
        final String path = cursor.getString(Fields.path.ordinal());
        final int size = cursor.getInt(Fields.size.ordinal());
        return new Track(id, path, size);
      }

      static ContentValues createValues(Track obj) {
        final ContentValues res = new ContentValues();
        res.put(Fields._id.name(), obj.id);
        res.put(Fields.path.name(), obj.path);
        res.put(Fields.size.name(), obj.size);
        return res;
      }
    }
    
    final static HashMap<String, Grouping> GroupTracks = new HashMap<String, Grouping>();
    
    static {
      for (String group : LIST) {
        GroupTracks.put(group, new Grouping(group + "_tracks", 32));
      }
    }
  }

  private final Helper helper;
  private final Timestamps timestamps;

  Database(Context context) {
    this.helper = Helper.create(context);
    this.timestamps = new Timestamps(helper);
  }

  final Transaction startTransaction() {
    return new Transaction(helper.getWritableDatabase());
  }

  final Timestamps.Lifetime getGroupsLifetime(String category, String filter, TimeStamp ttl) {
    return timestamps.getLifetime(category + filter, ttl);
  }
  
  final Timestamps.Lifetime getGroupTracksLifetime(String category, int id, TimeStamp ttl) {
    return timestamps.getLifetime(category + id, ttl);
  }
  
  final boolean queryGroups(String category, String filter, Catalog.GroupsVisitor visitor) {
    Log.d(TAG, "query%S(filter=%s)", category, filter);
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = filter.equals("#")
      ? "SUBSTR(" + Tables.Groups.Fields.name + ", 1, 1) NOT BETWEEN 'A' AND 'Z' COLLATE NOCASE"
      : Tables.Groups.Fields.name + " LIKE '" + filter + "%'";
    final Cursor cursor = db.query(category, null, selection, null, null, null, null);
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        visitor.setCountHint(count);
        while (cursor.moveToNext()) {
          visitor.accept(Tables.Groups.createGroup(cursor));
        }
        return true;
      }
    } finally {
      cursor.close();
    }
    return false;
  }

  final Group queryGroup(String category, int id) {
    Log.d(TAG, "query%S(id=%d)", category, id);
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = Tables.Groups.Fields._id + " = " + id;
    final Cursor cursor = db.query(category, null, selection, null, null, null, null);
    try {
      return cursor.moveToNext()
        ? Tables.Groups.createGroup(cursor)
        : null;
    } finally {
      cursor.close();
    }
  }

  final void addGroup(String category, Group obj) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insertWithOnConflict(category, null/* nullColumnHack */, Tables.Groups.createValues(obj),
      SQLiteDatabase.CONFLICT_REPLACE);
  }

  final boolean queryTracks(String category, int id, Catalog.TracksVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = Tables.Tracks.getSelection(Tables.GroupTracks.get(category).getIdsSelection(id));
    final Cursor cursor = db.query(Tables.Tracks.NAME, null, selection, null, null, null, null);
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        visitor.setCountHint(count);
        while (cursor.moveToNext()) {
          visitor.accept(Tables.Tracks.createTrack(cursor));
        }
        return true;
      }
    } finally {
      cursor.close();
    }
    return false;
  }
  
  final Track findTrack(String category, int id, String filename) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = Tables.Tracks.getSelection(Tables.GroupTracks.get(category).getIdsSelection(id))
        + " AND " + Tables.Tracks.Fields.path + " LIKE ?";
    final String[] selectionArgs = {"%/" + filename};
    final Cursor cursor = db.query(Tables.Tracks.NAME, null, selection, selectionArgs, null, null, null);
    try {
      if (cursor.moveToFirst()) {
        return Tables.Tracks.createTrack(cursor);
      }
    } finally {
      cursor.close();
    }
    return null;
  }

  final void addTrack(Track obj) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insertWithOnConflict(Tables.Tracks.NAME, null/* nullColumnHack */, Tables.Tracks.createValues(obj),
            SQLiteDatabase.CONFLICT_REPLACE);
  }

  final void addGroupTrack(String category, int id, Track obj) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    final Grouping grouping = Tables.GroupTracks.get(category);
    db.insert(grouping.getTableName(), null/* nullColumnHack */, grouping.createValues(id, obj.id));
  }

  private static class Helper extends SQLiteOpenHelper {

    static Helper create(Context context) {
      return new Helper(context);
    }

    private Helper(Context context) {
      super(context, NAME, null, VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
      Log.d(TAG, "Creating database");
      for (String table : Tables.LIST) {
        db.execSQL(Tables.Groups.getCreateQuery(table));
        db.execSQL(Tables.GroupTracks.get(table).createQuery());
      }
      db.execSQL(Tables.Tracks.CREATE_QUERY);
      db.execSQL(Timestamps.CREATE_QUERY);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
      Log.d(TAG, "Upgrading database %d -> %d", oldVersion, newVersion);
      Utils.cleanupDb(db);
      onCreate(db);
    }
  }
}
