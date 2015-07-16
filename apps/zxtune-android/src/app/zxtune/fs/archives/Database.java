/**
 *
 * @file
 *
 * @brief Playlist DAL
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.archives;

import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteStatement;
import android.net.Uri;
import android.util.Log;

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
 */

class Database {

  private static final String NAME = "archives";
  private static final int VERSION = 2;
  private static final String TAG = Database.class.getName();

  static final class Tables {
  
    private static final String DROP_TABLE_QUERY = "DROP TABLE IF EXISTS %s";
    private static final String DROP_VIEW_QUERY = "DROP VIEW IF EXISTS %s";
    private static final String DROP_TRIGGER_QUERY = "DROP TRIGGER IF EXISTS %s";
    private static final String DROP_INDEX_QUERY = "DROP INDEX IF EXISTS %s";
    
    static final class Paths {
      static enum Fields {
        path
      }
      
      private static final String NAME = "paths";
      private static final String CREATE_QUERY = "CREATE TABLE paths (" +
        "path TEXT PRIMARY KEY NOT NULL" +
        ");";
    }
    
    static final class ArchivesInternal {
      static enum Fields {
        path_id, modules
      }
      
      private static final String NAME = "archives_internal";
      private static final String CREATE_QUERY = "CREATE TABLE archives_internal (" +
        "path_id INTEGER NOT NULL PRIMARY KEY, " +
        "modules INTEGER NOT NULL" +
        ");";
    }
    
    static final class Archives {
      static enum Fields {
        path, modules
      }

      private static final String NAME = "archives";
      private static final String CREATE_QUERY =
        "CREATE VIEW archives AS " +
          "SELECT path, modules FROM archives_internal, paths " +
            "WHERE archives_internal.path_id = paths.rowid;";

      static final class InsertTrigger {
        private static final String NAME = "archives_insert";
        private static final String CREATE_QUERY =
          "CREATE TRIGGER archives_insert INSTEAD OF INSERT ON archives " +
            "FOR EACH ROW " +
            "BEGIN " +
              "INSERT OR IGNORE INTO paths(path) VALUES(new.path);" +
              "INSERT INTO archives_internal " +
                "SELECT rowid, new.modules FROM paths WHERE path = new.path;" +
            "END;";
      };
      
      private static final String INSERT_STATEMENT = 
        "INSERT INTO archives VALUES (?,?);";
    }
    
    static final class TracksInternal {
      static enum Fields {
        path_id, description, duration
      }
      
      private static final String NAME = "tracks_internal";
      private static final String CREATE_QUERY = "CREATE TABLE tracks_internal (" +
        "path_id INTEGER NOT NULL PRIMARY KEY, " +
        "description TEXT, " +
        "duration INTEGER NOT NULL" +
        ");";
    }
    
    static final class Tracks {
      static enum Fields {
        path, description, duration
      }
      
      private static final String NAME = "tracks";
      private static final String CREATE_QUERY =
        "CREATE VIEW tracks AS " +
          "SELECT path, description, duration FROM tracks_internal, paths " +
            "WHERE tracks_internal.path_id = paths.rowid;";
            
      static final class InsertTrigger {
        private static final String NAME = "tracks_insert";
        private static final String CREATE_QUERY =
          "CREATE TRIGGER tracks_insert INSTEAD OF INSERT ON tracks " +
            "FOR EACH ROW " +
            "BEGIN " +
              "INSERT OR IGNORE INTO paths(path) VALUES(new.path);" +
              "INSERT INTO tracks_internal " +
                "SELECT rowid, new.description, new.duration FROM paths WHERE path = new.path;" +
            "END;";
      };
      
      private static final String INSERT_STATEMENT = 
          "INSERT INTO tracks VALUES (?,?,?);";
    }
    
    static final class DirsInternal {
      static enum Fields {
        path_id, parent_id
      }
      
      private static final String NAME = "dirs_internal";
      private static final String CREATE_QUERY = "CREATE TABLE dirs_internal (" +
        "path_id INTEGER NOT NULL PRIMARY KEY, " +
        "parent_id INTEGER NOT NULL" +
        ");";
        
      static final class Index {
        private static final String NAME = "dirs_index";
        private static final String CREATE_QUERY = 
          "CREATE INDEX dirs_index ON dirs_internal (parent_id);";
      };
    }
    
    static final class Dirs {
      static enum Fields {
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
        private static final String NAME = "dirs_insert";
        private static final String CREATE_QUERY =
          "CREATE TRIGGER dirs_insert INSTEAD OF INSERT ON dirs " +
            "FOR EACH ROW " +
            "BEGIN " +
              //sqlite prior to 3.7.11 does not support multiple rows insert
              "INSERT OR IGNORE INTO paths(path) VALUES (new.path);" +
              "INSERT OR IGNORE INTO paths(path) VALUES (new.parent);" +
              "INSERT INTO dirs_internal " +
                "SELECT self.rowid, parent.rowid FROM paths AS self, paths AS parent " +
                  "WHERE self.path = new.path AND parent.path = new.parent;" +
            "END;";
      };
      
      private static final String INSERT_STATEMENT = 
        "INSERT INTO dirs VALUES (?,?);";
    }
    
    // tracks should be joined instead of intersection
    static final class Entries {
      static enum Fields {
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

  private final DBHelper dbHelper;

  Database(Context context) {
    this.dbHelper = new DBHelper(context);
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
    return new Transaction(dbHelper.getWritableDatabase());
  }
  
  final Cursor queryArchive(Uri path) {
    Log.d(TAG, "queryArchive(" + path + ") called");
    final SQLiteDatabase db = dbHelper.getReadableDatabase();
    final String selection = Tables.Archives.Fields.path + " = ?";
    final String[] selectionArgs = {path.toString()};
    return db.query(Tables.Archives.NAME, null, selection, selectionArgs, null/* groupBy */,
        null/* having */, null/*orderBy*/);
  }
  
  final Cursor queryInfo(Uri path) {
    Log.d(TAG, "queryInfo(" + path + ") called");
    final SQLiteDatabase db = dbHelper.getReadableDatabase();
    final String selection = Tables.Entries.Fields.path + " = ?";
    final String[] selectionArgs = {path.toString()};
    return db.query(Tables.Entries.NAME, null, selection, selectionArgs, null/* groupBy */,
        null/* having */, null/*orderBy*/);
  }

  final Cursor queryListing(Uri path) {
    Log.d(TAG, "queryListing(" + path + ") called");
    final SQLiteDatabase db = dbHelper.getReadableDatabase();
    final String selection = Tables.Entries.Fields.parent + " = ?";
    final String[] selectionArgs = {path.toString()};
    return db.query(Tables.Entries.NAME, null, selection, selectionArgs, null/* groupBy */,
        null/* having */, null/*orderBy*/);
  }

  final long addArchive(Archive arch) {
    Log.d(TAG, "addArchive(" + arch.path + ", " + arch.modules + " modules) called");
    final SQLiteStatement stat = dbHelper.getInsertArchiveStatement();
    stat.clearBindings();
    stat.bindString(1 + Tables.Archives.Fields.path.ordinal(), arch.path.toString());
    stat.bindLong(1 + Tables.Archives.Fields.modules.ordinal(), arch.modules);
    return stat.executeInsert();
  }

  final long addTrack(Track track) {
    final SQLiteStatement stat = dbHelper.getInsertTrackStatement();
    stat.clearBindings();
    stat.bindString(1 + Tables.Tracks.Fields.path.ordinal(), track.path.toString());
    stat.bindString(1 + Tables.Tracks.Fields.description.ordinal(), track.description);
    stat.bindLong(1 + Database.Tables.Tracks.Fields.duration.ordinal(), track.duration.convertTo(TimeUnit.MILLISECONDS));
    return stat.executeInsert();
  }
  
  final long addDirEntry(DirEntry entry) {
    final SQLiteStatement stat = dbHelper.getInsertDirStatement();
    stat.clearBindings();
    stat.bindString(1 + Tables.Dirs.Fields.path.ordinal(), entry.path.toString());
    stat.bindString(1 + Tables.Dirs.Fields.parent.ordinal(), entry.parent.toString());
    return stat.executeInsert();
  }
  
  private static class DBHelper extends SQLiteOpenHelper {
    
    private SQLiteStatement insertArchiveStatement;
    private SQLiteStatement insertTrackStatement;
    private SQLiteStatement insertDirStatement;

    DBHelper(Context context) {
      super(context, NAME, null, VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
      Log.d(TAG, "Creating database");
      final String QUERIES[] = {
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
      Log.d(TAG, String.format("Upgrading database %d -> %d", oldVersion, newVersion));
      onDelete(db);
      onCreate(db);
    }
    
    private void onDelete(SQLiteDatabase db) {
      final String TABLES[] = {
        Tables.Paths.NAME,
        Tables.ArchivesInternal.NAME,
        Tables.TracksInternal.NAME,
        Tables.DirsInternal.NAME
      };
      final String VIEWS[] = {
        Tables.Archives.NAME,
        Tables.Tracks.NAME,
        Tables.Dirs.NAME,
        Tables.Entries.NAME
      };
      final String TRIGGERS[] = {
        Tables.Archives.InsertTrigger.NAME,
        Tables.Tracks.InsertTrigger.NAME,
        Tables.Dirs.InsertTrigger.NAME
      };
      final String INDICES[] = {
        Tables.DirsInternal.Index.NAME
      };
      
      for (String table : TABLES) {
        db.execSQL(String.format(Tables.DROP_TABLE_QUERY, table));
      }
      for (String view : VIEWS) {
        db.execSQL(String.format(Tables.DROP_VIEW_QUERY, view));
      }
      for (String trigger : TRIGGERS) {
        db.execSQL(String.format(Tables.DROP_TRIGGER_QUERY, trigger));
      }
      for (String index : INDICES) {
        db.execSQL(String.format(Tables.DROP_INDEX_QUERY, index));
      }
    }
    
    public final SQLiteStatement getInsertArchiveStatement() {
      if (insertArchiveStatement == null) {
        final SQLiteDatabase db = getWritableDatabase();
        insertArchiveStatement = db.compileStatement(Tables.Archives.INSERT_STATEMENT);
      }
      return insertArchiveStatement;
    }

    public final SQLiteStatement getInsertTrackStatement() {
      if (insertTrackStatement == null) {
        final SQLiteDatabase db = getWritableDatabase();
        insertTrackStatement = db.compileStatement(Tables.Tracks.INSERT_STATEMENT);
      }
      return insertTrackStatement;
    }

    public final SQLiteStatement getInsertDirStatement() {
      if (insertDirStatement == null) {
        final SQLiteDatabase db = getWritableDatabase();
        insertDirStatement = db.compileStatement(Tables.Dirs.INSERT_STATEMENT);
      }
      return insertDirStatement;
    }
  }
}
