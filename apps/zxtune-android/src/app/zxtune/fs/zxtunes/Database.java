/**
 *
 * @file
 *
 * @brief DAL helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.zxtunes;

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
 * CREATE TABLE authors (_id INTEGER PRIMARY KEY, nickname TEXT NOT NULL, name TEXT)
 * CREATE TABLE tracks (_id INTEGER PRIMARY KEY, filename TEXT NOT NULL, title TEXT, duration
 * INTEGER, date INTEGER)
 * CREATE TABLE author_tracks (hash INTEGER UNIQUE, author INTEGER, track INTEGER)
 * 
 * use hash as 100000 * author + track to support multiple insertings of same pair
 * 
 * Version 2
 * 
 * Use author_tracks standard grouping
 * Use timestamps
 */

final class Database {

  final static String TAG = Database.class.getName();

  final static String NAME = "www.zxtunes.com";
  final static int VERSION = 2;

  final static class Tables {

    final static class Authors {

      static enum Fields {
        _id, nickname, name
      }

      final static String NAME = "authors";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
          + " INTEGER PRIMARY KEY, " + Fields.nickname + " TEXT NOT NULL, " + Fields.name
          + " TEXT);";

      static String getSelection(int id) {
        return Fields._id + " = " + id;
      }
    }

    final static class Tracks {

      static enum Fields {
        _id, filename, title, duration, date
      }

      final static String NAME = "tracks";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
          + " INTEGER PRIMARY KEY, " + Fields.filename + " TEXT NOT NULL, " + Fields.title
          + " TEXT, " + Fields.duration + " INTEGER, " + Fields.date + " INTEGER);";

      static String getSelection(int id) {
        return Fields._id + " = " + id;
      }
      
      static String getSelection(String subquery) {
        return Fields._id + " IN (" + subquery + ")";
      }
    }

    final static class AuthorsTracks {

      final static String NAME = "authors_tracks";
      private final static Grouping grouping = new Grouping(NAME, 32);

      static String createQuery() {
        return grouping.createQuery();
      }
      
      static ContentValues createValues(Author author, Track track) {
        return grouping.createValues(author.id, track.id);
      }
      
      static String getTracksIdsSelection(Author author) {
        return grouping.getIdsSelection(author.id);
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

  final Timestamps.Lifetime getAuthorsLifetime(TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Authors.NAME, ttl);
  }
  
  final Timestamps.Lifetime getAuthorTracksLifetime(Author author, TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Authors.NAME + author.id, ttl);
  }
  
  final boolean queryAuthors(Integer id, Catalog.AuthorsVisitor visitor) {
    Log.d(TAG, "queryAuthors(id=%d)", id);
    final String selection = id != null ? Tables.Authors.getSelection(id) : null;
    final SQLiteDatabase db = helper.getReadableDatabase();
    final Cursor cursor = db.query(Tables.Authors.NAME, null, selection, null, null, null, null);
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        visitor.setCountHint(count);
        while (cursor.moveToNext()) {
          visitor.accept(createAuthor(cursor));
        }
        return true;
      }
    } finally {
      cursor.close();
    }
    return false;
  }

  final void addAuthor(Author obj) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insertWithOnConflict(Tables.Authors.NAME, null/* nullColumnHack */, createValues(obj),
        SQLiteDatabase.CONFLICT_REPLACE);
  }

  final boolean queryTrack(int id, Catalog.TracksVisitor visitor) {
    Log.d(TAG, "queryTracks(id=%d)", id);
    final String selection = Tables.Tracks.getSelection(id);
    return queryTracks(selection, visitor);
  }
  
  final boolean queryAuthorTracks(Author author, Catalog.TracksVisitor visitor) {
    Log.d(TAG, "queryTracks(author=%d)", author.id);
    final String selection = Tables.Tracks.getSelection(Tables.AuthorsTracks.getTracksIdsSelection(author));
    return queryTracks(selection, visitor);
  }
  
  private boolean queryTracks(String selection, Catalog.TracksVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final Cursor cursor = db.query(Tables.Tracks.NAME, null, selection, null, null, null, null);
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        visitor.setCountHint(count);
        while (cursor.moveToNext()) {
          visitor.accept(createTrack(cursor));
        }
        return true;
      }
    } finally {
      cursor.close();
    }
    return false;
  }

  final void addTrack(Track obj) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insertWithOnConflict(Tables.Tracks.NAME, null/* nullColumnHack */, createValues(obj),
        SQLiteDatabase.CONFLICT_REPLACE);
  }
  
  final void addAuthorTrack(Author author, Track track) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insert(Tables.AuthorsTracks.NAME, null/* nullColumnHack */, Tables.AuthorsTracks.createValues(author, track));
  }

  private static Author createAuthor(Cursor cursor) {
    final int id = cursor.getInt(Tables.Authors.Fields._id.ordinal());
    final String name = cursor.getString(Tables.Authors.Fields.name.ordinal());
    final String nickname = cursor.getString(Tables.Authors.Fields.nickname.ordinal());
    return new Author(id, nickname, name);
  }

  private static ContentValues createValues(Author obj) {
    final ContentValues res = new ContentValues();
    res.put(Tables.Authors.Fields._id.name(), obj.id);
    res.put(Tables.Authors.Fields.name.name(), obj.name);
    res.put(Tables.Authors.Fields.nickname.name(), obj.nickname);
    return res;
  }

  private static Track createTrack(Cursor cursor) {
    final int id = cursor.getInt(Tables.Tracks.Fields._id.ordinal());
    final String filename = cursor.getString(Tables.Tracks.Fields.filename.ordinal());
    final String title = cursor.getString(Tables.Tracks.Fields.title.ordinal());
    final int duration = cursor.getInt(Tables.Tracks.Fields.duration.ordinal());
    final int date = cursor.getInt(Tables.Tracks.Fields.date.ordinal());
    return new Track(id, filename, title, duration, date);
  }

  private static ContentValues createValues(Track obj) {
    final ContentValues res = new ContentValues();
    res.put(Tables.Tracks.Fields._id.name(), obj.id);
    res.put(Tables.Tracks.Fields.filename.name(), obj.filename);
    res.put(Tables.Tracks.Fields.title.name(), obj.title);
    res.put(Tables.Tracks.Fields.duration.name(), obj.duration);
    res.put(Tables.Tracks.Fields.date.name(), obj.date);
    return res;
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
      db.execSQL(Tables.Authors.CREATE_QUERY);
      db.execSQL(Tables.Tracks.CREATE_QUERY);
      db.execSQL(Tables.AuthorsTracks.createQuery());
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
