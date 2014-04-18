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

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.util.Log;

/**
 * Version 1
 *
 * CREATE TABLE {authors,collections,formats} (_id INTEGER PRIMARY KEY, name TEXT NOT NULL, tracks INTEGER)
 * CREATE TABLE tracks (_id INTEGER PRIMARY KEY, path TEXT NOT NULL, size INTEGER)
 * CREATE TABLE {authors,collections,formats}_tracks (hash INTEGER UNIQUE, group_id INTEGER, track_id INTEGER)
 * CREATE TABLE collections_tracks (hash INTEGER UNIQUE, collection INTEGER, track INTEGER)
 *
 * use hash as 10000000000 * group_id + track_id to support multiple insertions of same pair
 */

final class Database {

  final static String TAG = Database.class.getName();

  final static String NAME = "modland.com";
  final static int VERSION = 1;

  final static class Tables {

    final static String DROP_QUERY = "DROP TABLE ? IF EXISTS;";

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

    final static class GroupTracks {

      static enum Fields {
        hash, group_id, track_id
      }

      final static String getName(String table) {
        return table + "_tracks";
      }

      final static String getCreateQuery(String table) {
        return "CREATE TABLE " + getName(table) + " (" + Fields.hash
              + " INTEGER UNIQUE, " + Fields.group_id + " INTEGER, " + Fields.track_id + " INTEGER);";
      }

      static ContentValues createValues(int group, long track) {
        final long hash = 10000000000l * group + track;
        final ContentValues res = new ContentValues();
        res.put(Fields.hash.name(), hash);
        res.put(Fields.group_id.name(), group);
        res.put(Fields.track_id.name(), track);
        return res;
      }
    }
  }

  private final Helper helper;

  Database(Context context) {
    this.helper = Helper.create(context);
  }

  class Transaction {

    private final SQLiteDatabase db;

    Transaction(SQLiteDatabase db) {
      this.db = db;
      db.beginTransaction();
    }

    final void succeed() {
      db.setTransactionSuccessful();
    }

    final void finish() {
      db.endTransaction();
    }
  }

  final Transaction startTransaction() {
    return new Transaction(helper.getWritableDatabase());
  }

  final void queryGroups(String category, String filter, Catalog.GroupsVisitor visitor) {
    Log.d(TAG, "query" + category + "(filter=" + filter + ")");
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
      }
    } finally {
      cursor.close();
    }
  }

  final Group queryGroup(String category, int id) {
    Log.d(TAG, "query" + category + "(id=" + id + ")");
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

  final void queryTracks(String category, int id, Catalog.TracksVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = createGroupTracksSelection(category, id);
    final Cursor cursor = db.query(Tables.Tracks.NAME, null, selection, null, null, null, null);
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        visitor.setCountHint(count);
        while (cursor.moveToNext()) {
          visitor.accept(Tables.Tracks.createTrack(cursor));
        }
      }
    } finally {
      cursor.close();
    }
  }

  private static String createGroupTracksSelection(String category, int id) {
    final String idQuery =
      SQLiteQueryBuilder.buildQueryString(true, Tables.GroupTracks.getName(category),
        new String[]{Tables.GroupTracks.Fields.track_id.name()}, Tables.GroupTracks.Fields.group_id + " = "
        + id, null, null, null, null);
    return Tables.Tracks.Fields._id + " IN (" + idQuery + ")";
  }

  final void addTrack(Track obj) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insertWithOnConflict(Tables.Tracks.NAME, null/* nullColumnHack */, Tables.Tracks.createValues(obj),
            SQLiteDatabase.CONFLICT_REPLACE);
  }

  final void addGroupTrack(String category, int id, Track obj) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insert(Tables.GroupTracks.getName(category), null/* nullColumnHack */, Tables.GroupTracks.createValues(id, obj.id));
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
        db.execSQL(Tables.GroupTracks.getCreateQuery(table));
      }
      db.execSQL(Tables.Tracks.CREATE_QUERY);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
      Log.d(TAG, String.format("Upgrading database %d -> %d", oldVersion, newVersion));
      for (String table : Tables.LIST) {
        db.execSQL(Tables.DROP_QUERY, new Object[] {table});
        db.execSQL(Tables.DROP_QUERY, new Object[] {Tables.GroupTracks.getName(table)});
      }
      db.execSQL(Tables.DROP_QUERY, new Object[] {Tables.Tracks.NAME});
      onCreate(db);
    }
  }
}
