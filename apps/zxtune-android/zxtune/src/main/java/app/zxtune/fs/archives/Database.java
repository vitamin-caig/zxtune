/**
 * @file
 * @brief Playlist DAL
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.archives;

import android.content.Context;
import android.database.Cursor;
import android.database.DatabaseUtils;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.net.Uri;

import java.io.IOException;
import java.util.List;

import app.zxtune.BuildConfig;
import app.zxtune.Log;
import app.zxtune.fs.dbhelpers.DBProvider;
import app.zxtune.fs.dbhelpers.Objects;
import app.zxtune.fs.dbhelpers.Utils;

/*
 * Archived content DB model.
 */

/*
 * Version 1
 */

/*
 * Table 'paths' - deduplicated paths:
 * '_id', integer primary key autoincrement - unique identifier
 * 'path', text unique not null - value
 * 
 * Required to dedup base according to investigations:
 * 
 * 22535 entries
 * average path len is 74.7
 * average parent len is 57.7
 * unique parents is 1318
 * unique paths+parents is 22539
 */

/*
 * Table 'archives_internal' - all browsed archives
 * 'path_id', integer primary key not null - identifier from 'paths' table
 * 'modules', integer not null - modules count in specified archive
 * 
 * Table 'archives' - virtual table:
 * 'path'
 * 'modules'
 */

/*
 * Table 'tracks_internal' - all found tracks
 * 'path_id', integer primary key not null - identifier from 'paths' table
 * 'description', text - metainfo
 * 'duration', integer not null - duration in msec
 * 
 * Table 'tracks' - virtual table:
 * 'path'
 * 'description'
 * 'duration'
 */

/*
 * Table 'dirs_internal' - hierarchy relations
 * 'path_id', integer primary key not null - identifier from 'paths' table
 * 'parent_id', integer not null - identifier from 'paths' table
 * 
 * Table 'dirs' - virtual table
 * 'path'
 * 'parent'
 */

/*
 * Table 'entries' - virtual table on dirs+tracks
 * 'path'
 * 'parent' 
 * 'description'
 * 'duration'
 */
 
/*
 * Version 2
 *  added 7zip support
 *  
 * Version 3
 *  use REPLACE instead of INSERT in views insert triggers
 *
 * Version 4
 *  set of new formats supported
 *
 * After app version code is used as an archive version
 */

class Database {

  private static final String NAME = "archives";
  private static final int VERSION = BuildConfig.VERSION;
  private static final String TAG = Database.class.getName();

  static final class Tables {

    static final class Paths {
      enum Fields {
        path
      }

      private static final String CREATE_QUERY = "CREATE TABLE paths (" +
              "path TEXT PRIMARY KEY NOT NULL" +
              ");";
    }

    static final class ArchivesInternal {
      enum Fields {
        path_id, modules
      }

      private static final String CREATE_QUERY = "CREATE TABLE archives_internal (" +
              "path_id INTEGER NOT NULL PRIMARY KEY, " +
              "modules INTEGER NOT NULL" +
              ");";
    }
    
    /*
     * Working on views with compiled REPLACE statement breaks rowid numbering in paths table.
     * So use explicit INSERT statement.
     */

    static final class Archives extends Objects {
      enum Fields {
        path, modules
      }

      private static final String NAME = "archives";
      private static final String CREATE_QUERY =
              "CREATE VIEW archives AS " +
                      "SELECT path, modules FROM archives_internal, paths " +
                      "WHERE archives_internal.path_id = paths.rowid;";

      static final class InsertTrigger {
        private static final String CREATE_QUERY =
                "CREATE TRIGGER archives_insert INSTEAD OF INSERT ON archives " +
                        "FOR EACH ROW " +
                        "BEGIN " +
                        "INSERT OR IGNORE INTO paths(path) VALUES(new.path);" +
                        "REPLACE INTO archives_internal " +
                        "SELECT rowid, new.modules FROM paths WHERE path = new.path;" +
                        "END;";
      }

      Archives(DBProvider helper) {
        super(helper, NAME, "INSERT", Fields.values().length);
      }

      void add(Archive obj) {
        add(obj.path.toString(), obj.modules);
      }
    }

    static final class TracksInternal {
      enum Fields {
        path_id, description, duration
      }

      private static final String CREATE_QUERY = "CREATE TABLE tracks_internal (" +
              "path_id INTEGER NOT NULL PRIMARY KEY, " +
              "description TEXT, " +
              "duration INTEGER NOT NULL" +
              ");";
    }

    static final class Tracks extends Objects {
      enum Fields {
        path, description, duration
      }

      private static final String NAME = "tracks";
      private static final String CREATE_QUERY =
              "CREATE VIEW tracks AS " +
                      "SELECT path, description, duration FROM tracks_internal, paths " +
                      "WHERE tracks_internal.path_id = paths.rowid;";

      static final class InsertTrigger {
        private static final String CREATE_QUERY =
                "CREATE TRIGGER tracks_insert INSTEAD OF INSERT ON tracks " +
                        "FOR EACH ROW " +
                        "BEGIN " +
                        "INSERT OR IGNORE INTO paths(path) VALUES(new.path);" +
                        "REPLACE INTO tracks_internal " +
                        "SELECT rowid, new.description, new.duration FROM paths WHERE path = new.path;" +
                        "END;";
      }

      Tracks(DBProvider helper) {
        super(helper, NAME, "INSERT", Fields.values().length);
      }

      void add(Track obj) {
        add(obj.path.toString(), obj.description, obj.duration.toMilliseconds());
      }
    }

    static final class DirsInternal {
      enum Fields {
        path_id, parent_id
      }

      private static final String CREATE_QUERY = "CREATE TABLE dirs_internal (" +
              "path_id INTEGER NOT NULL PRIMARY KEY, " +
              "parent_id INTEGER NOT NULL" +
              ");";

      static final class Index {
        private static final String CREATE_QUERY =
                "CREATE INDEX dirs_index ON dirs_internal (parent_id);";
      }
    }

    static final class Dirs extends Objects {
      enum Fields {
        path, parent
      }

      private static final String NAME = "dirs";
      private static final String CREATE_QUERY =
              "CREATE VIEW dirs AS " +
                      "SELECT " +
                      "self.path AS path, " +
                      "parent.path AS parent " +
                      "FROM dirs_internal, paths AS self, paths AS parent " +
                      "WHERE dirs_internal.path_id = self.rowid AND dirs_internal.parent_id = parent.rowid;";

      static final class InsertTrigger {
        private static final String CREATE_QUERY =
                "CREATE TRIGGER dirs_insert INSTEAD OF INSERT ON dirs " +
                        "FOR EACH ROW " +
                        "BEGIN " +
                        //sqlite prior to 3.7.11 does not support multiple rows insert
                        "INSERT OR IGNORE INTO paths(path) VALUES (new.path);" +
                        "INSERT OR IGNORE INTO paths(path) VALUES (new.parent);" +
                        "REPLACE INTO dirs_internal " +
                        "SELECT self.rowid, parent.rowid FROM paths AS self, paths AS parent " +
                        "WHERE self.path = new.path AND parent.path = new.parent;" +
                        "END;";
      }

      Dirs(DBProvider helper) {
        super(helper, NAME, "INSERT", Fields.values().length);
      }

      void add(DirEntry dir) {
        add(dir.path.toString(), dir.parent.toString());
      }
    }

    // tracks should be joined instead of intersection
    static final class Entries {
      enum Fields {
        path, parent, description, duration
      }

      private static final String NAME = "entries";
      private static final String CREATE_QUERY = "CREATE VIEW entries AS " +
              "SELECT " +
              "dirs.path AS path, " +
              "dirs.parent AS parent, " +
              "tracks.description AS description, " +
              "tracks.duration AS duration " +
              "FROM dirs LEFT OUTER JOIN tracks ON dirs.path = tracks.path;";
    }
  }

  private final DBProvider dbHelper;
  private final Tables.Archives archives;
  private final Tables.Dirs dirs;
  private final Tables.Tracks tracks;

  Database(Context context) {
    this.dbHelper = new DBProvider(new DBHelper(context));
    this.archives = new Tables.Archives(dbHelper);
    this.dirs = new Tables.Dirs(dbHelper);
    this.tracks = new Tables.Tracks(dbHelper);
  }

  final void runInTransaction(Utils.ThrowingRunnable cmd) throws IOException {
    Utils.runInTransaction(dbHelper, cmd);
  }

  final Cursor queryArchive(Uri path) {
    Log.d(TAG, "queryArchive(%s)", path);
    final SQLiteDatabase db = dbHelper.getReadableDatabase();
    final String selection = Tables.Archives.Fields.path + " = ?";
    final String[] selectionArgs = {path.toString()};
    return db.query(Tables.Archives.NAME, null, selection, selectionArgs, null/* groupBy */,
            null/* having */, null/*orderBy*/);
  }

  final Cursor queryArchives(List<Uri> paths) {
    Log.d(TAG, "queryArchives(%d items)", paths.size());
    final SQLiteDatabase db = dbHelper.getReadableDatabase();
    final StringBuilder builder = new StringBuilder(paths.size() * 50);
    builder.append(Tables.Archives.Fields.path);
    builder.append(" IN (");
    for (int i = 0, lim = paths.size(); i != lim; ++i) {
      if (i != 0) {
        builder.append(',');
      }
      DatabaseUtils.appendEscapedSQLString(builder, paths.get(i).toString());
    }
    builder.append(")");
    return db.query(Tables.Archives.NAME, null, builder.toString(), null, null/* groupBy */,
        null/* having */, null/*orderBy*/);
  }

  final Cursor queryInfo(Uri path) {
    Log.d(TAG, "queryInfo(%s)", path);
    final SQLiteDatabase db = dbHelper.getReadableDatabase();
    final String selection = Tables.Entries.Fields.path + " = ?";
    final String[] selectionArgs = {path.toString()};
    return db.query(Tables.Entries.NAME, null, selection, selectionArgs, null/* groupBy */,
            null/* having */, null/*orderBy*/);
  }

  final Cursor queryListing(Uri path) {
    Log.d(TAG, "queryListing(%s)", path);
    final SQLiteDatabase db = dbHelper.getReadableDatabase();
    final String selection = Tables.Entries.Fields.parent + " = ?";
    final String[] selectionArgs = {path.toString()};
    return db.query(Tables.Entries.NAME, null, selection, selectionArgs, null/* groupBy */,
            null/* having */, null/*orderBy*/);
  }

  final void addArchive(Archive arch) {
    Log.d(TAG, "addArchive(%s, %d modules)", arch.path, arch.modules);
    archives.add(arch);
  }

  final void addTrack(Track track) {
    tracks.add(track);
  }

  final void addDirEntry(DirEntry entry) {
    dirs.add(entry);
  }

  private static class DBHelper extends SQLiteOpenHelper {

    DBHelper(Context context) {
      super(context, NAME, null, VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
      Log.d(TAG, "Creating database");
      final String[] QUERIES = {
              Tables.Paths.CREATE_QUERY,
              Tables.ArchivesInternal.CREATE_QUERY,
              Tables.Archives.CREATE_QUERY, Tables.Archives.InsertTrigger.CREATE_QUERY,
              Tables.TracksInternal.CREATE_QUERY,
              Tables.Tracks.CREATE_QUERY, Tables.Tracks.InsertTrigger.CREATE_QUERY,
              Tables.DirsInternal.CREATE_QUERY, Tables.DirsInternal.Index.CREATE_QUERY,
              Tables.Dirs.CREATE_QUERY, Tables.Dirs.InsertTrigger.CREATE_QUERY,
              Tables.Entries.CREATE_QUERY
      };
      for (String query : QUERIES) {
        db.execSQL(query);
      }
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
      Log.d(TAG, "Upgrading database %d -> %d", oldVersion, newVersion);
      Utils.cleanupDb(db);
      onCreate(db);
    }
  }
}
