/**
 * @file
 * @brief DAL helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modland;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.net.Uri;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.HashMap;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.DBProvider;
import app.zxtune.fs.dbhelpers.Grouping;
import app.zxtune.fs.dbhelpers.Objects;
import app.zxtune.fs.dbhelpers.Timestamps;
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
 */

class Database {

  private static final String TAG = Database.class.getName();

  private static final String NAME = "modland.com";
  private static final int VERSION = 2;

  static final class Tables {

    static final class Groups extends Objects {

      enum Fields {
        _id, name, tracks
      }

      static String getCreateQuery(String name) {
        return "CREATE TABLE " + name + " (" + Fields._id
                + " INTEGER PRIMARY KEY, " + Fields.name + " TEXT NOT NULL, " + Fields.tracks
                + " INTEGER);";
      }

      Groups(DBProvider helper, String name) {
        super(helper, name, Fields.values().length);
      }

      final void add(Group obj) {
        add(obj.getId(), obj.getName(), obj.getTracks());
      }

      static Group createGroup(Cursor cursor) {
        final int id = cursor.getInt(Fields._id.ordinal());
        final String title = cursor.getString(Fields.name.ordinal());
        final int tracks = cursor.getInt(Fields.tracks.ordinal());
        return new Group(id, title, tracks);
      }
    }

    static final class Authors {

      static final String NAME = "authors";
    }

    static final class Collections {

      static final String NAME = "collections";
    }

    static final class Formats {

      static final String NAME = "formats";
    }

    static final String[] LIST = {Authors.NAME, Collections.NAME, Formats.NAME};

    static final class Tracks extends Objects {

      enum Fields {
        _id, path, size
      }

      static final String NAME = "tracks";

      static final String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
              + " INTEGER PRIMARY KEY, " + Fields.path + " TEXT NOT NULL, " + Fields.size + " INTEGER);";

      Tracks(DBProvider helper) {
        super(helper, NAME, Fields.values().length);
      }

      final void add(Track obj) {
        add(obj.getId(), obj.getPath(), obj.getSize());
      }

      static Track createTrack(Cursor cursor) {
        final int id = cursor.getInt(Fields._id.ordinal());
        final String path = cursor.getString(Fields.path.ordinal());
        final int size = cursor.getInt(Fields.size.ordinal());
        return new Track(id, path, size);
      }

      static String getSelection(String subquery) {
        return Fields._id + " IN (" + subquery + ")";
      }
    }

    static final class GroupTracks extends Grouping {

      static String name(String category) {
        return category + "_tracks";
      }

      static String getCreateQuery(String category) {
        return createQuery(name(category));
      }

      GroupTracks(DBProvider helper, String category) {
        super(helper, name(category), 32);
      }
    }
  }

  private final DBProvider helper;
  private final HashMap<String, Tables.Groups> groups;
  private final HashMap<String, Tables.GroupTracks> groupTracks;
  private final Tables.Tracks tracks;
  private final Timestamps timestamps;

  Database(Context context) {
    this.helper = new DBProvider(Helper.create(context));
    this.groups = new HashMap<>();
    this.groupTracks = new HashMap<>();
    for (String group : Tables.LIST) {
      groups.put(group, new Tables.Groups(helper, group));
      groupTracks.put(group, new Tables.GroupTracks(helper, group));
    }
    this.tracks = new Tables.Tracks(helper);
    this.timestamps = new Timestamps(helper);
  }

  void runInTransaction(Utils.ThrowingRunnable cmd) throws IOException {
    Utils.runInTransaction(helper, cmd);
  }

  Timestamps.Lifetime getGroupsLifetime(String category, String filter, TimeStamp ttl) {
    return timestamps.getLifetime(category + filter, ttl);
  }

  Timestamps.Lifetime getGroupTracksLifetime(String category, int id, TimeStamp ttl) {
    return timestamps.getLifetime(category + id, ttl);
  }

  boolean queryGroups(String category, String filter, Catalog.GroupsVisitor visitor) {
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

  @Nullable
  Group queryGroup(String category, int id) {
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

  void addGroup(String category, Group obj) {
    final Tables.Groups group = groups.get(category);
    if (group != null) {
      group.add(obj);
    }
  }

  boolean queryTracks(String category, int id, Catalog.TracksVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = Tables.Tracks.getSelection(groupTracks.get(category).getIdsSelection(id));
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

  @Nullable
  Track findTrack(String category, int id, String filename) {
    final String encodedFilename = Uri.encode(filename).replace("!", "%21").replace("'", "%27").replace("(", "%28").replace(")", "%29");
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = Tables.Tracks.getSelection(groupTracks.get(category).getIdsSelection(id))
            + " AND " + Tables.Tracks.Fields.path + " LIKE ?";
    final String[] selectionArgs = {"%/" + encodedFilename};
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

  void addTrack(Track obj) {
    tracks.add(obj);
  }

  void addGroupTrack(String category, int id, Track obj) {
    final Tables.GroupTracks tracks = groupTracks.get(category);
    if (tracks != null) {
      tracks.add(id, obj.getId());
    }
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
