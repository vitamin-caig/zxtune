/**
 *
 * @file
 *
 * @brief DAL helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.amp;

import java.util.concurrent.TimeUnit;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.amp.Catalog.AuthorsVisitor;

/**
 * Version 1
 *
 * CREATE TABLE authors (_id INTEGER PRIMARY KEY, handle TEXT NOT NULL, real_name TEXT NOT NULL)
 * CREATE TABLE tracks (_id INTEGER PRIMARY KEY, filename TEXT NOT NULL, size INTEGER NOT NULL)
 * CREATE TABLE author_tracks (_id INTEGER PRIMARY KEY)
 *   with _id as (author_id << 32) | track_id to support multiple insertions of same pair
 *   
 * CREATE TABLE country_authors (_id INTEGER PRIMARY KEY, author_id INTEGER)
 *   with _id as (country_id << 32) | author_id to support multiple insertions of same pair
 *   
 * CREATE TABLE timestamps (_id TEXT PRIMARY KEY, timestamp DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL)
 * 
 */

final class Database {

  final static String TAG = Database.class.getName();

  final static String NAME = "amp.dascene.net";
  final static int VERSION = 1;

  final static class Tables {

    final static String DROP_QUERY = "DROP TABLE IF EXISTS %s;";
    
    final static class Authors {

      static enum Fields {
        _id, handle, real_name
      }
      
      final static String NAME = "authors";
      
      final static String CREATE_QUERY = "CREATE TABLE authors (" +
        "_id INTEGER PRIMARY KEY, " +
        "handle TEXT NOT NULL, " +
        "real_name TEXT NOT NULL" +
        ");";
      ;

      static ContentValues createValues(Author obj) {
        final ContentValues res = new ContentValues();
        res.put(Fields._id.name(), obj.id);
        res.put(Fields.handle.name(), obj.handle);
        res.put(Fields.real_name.name(), obj.realName);
        return res;
      }
      
      static Author createAuthor(Cursor cursor) {
        final int id = cursor.getInt(Fields._id.ordinal());
        final String handle = cursor.getString(Fields.handle.ordinal());
        final String realName = cursor.getString(Fields.real_name.ordinal());
        return new Author(id, handle, realName);
      }
    }

    final static class Tracks {

      static enum Fields {
        _id, filename, size
      }

      final static String NAME = "tracks";

      final static String CREATE_QUERY = "CREATE TABLE tracks (" +
          "_id INTEGER PRIMARY KEY, " +
          "filename TEXT NOT NULL, " +
          "size INTEGER NOT NULL" +
          ");";

      static Track createTrack(Cursor cursor) {
        final int id = cursor.getInt(Fields._id.ordinal());
        final String filename = cursor.getString(Fields.filename.ordinal());
        final int size = cursor.getInt(Fields.size.ordinal());
        return new Track(id, filename, size);
      }

      static ContentValues createValues(Track obj) {
        final ContentValues res = new ContentValues();
        res.put(Fields._id.name(), obj.id);
        res.put(Fields.filename.name(), obj.filename);
        res.put(Fields.size.name(), obj.size);
        return res;
      }
    }
    
    static class Grouping {
      static enum Fields {
        _id
      }
      
      protected static String getCreateQuery(String tableName) {
        return "CREATE TABLE " + tableName + " (_id INTEGER PRIMARY KEY);";
      }

      protected static ContentValues createValues(int group, int object) {
        final ContentValues res = new ContentValues();
        res.put(Fields._id.name(), getId(group, object));
        return res;
      }

      protected static String getIdsSelection(String tableName, int group) {
        final long lower = getId(group, 0);
        final long upper = getId(group + 1, 0) - 1;
        final String groupSelection = Fields._id + " BETWEEN " + lower + " AND " + upper;
        final String objectSelection = Fields._id + " & ((1 << 32) - 1)";
        return SQLiteQueryBuilder.buildQueryString(true, tableName,
              new String[]{objectSelection}, groupSelection, null, null, null, null);
      }

      private static long getId(int group, int object) {
        return ((long)group << 32) | object;
      }
    }

    final static class AuthorTracks extends Grouping {

      final static String NAME = "author_tracks";
      
      static String getCreateQuery() {
        return getCreateQuery(NAME);
      }
      
      static ContentValues createValues(Author author, Track track) {
        return createValues(author.id, track.id);
      }
      
      static String getTracksIdsSelection(Author author) {
        return getIdsSelection(NAME, author.id);
      }
    }

    final static class CountryAuthors extends Grouping {

      final static String NAME = "country_authors";
      
      static String getCreateQuery() {
        return getCreateQuery(NAME);
      }

      static ContentValues createValues(Country country, Author author) {
        return createValues(country.id, author.id);
      }
      
      static String getAuthorsIdsSelection(Country country) {
        return getIdsSelection(NAME, country.id);
      }
    }

    final static class Timestamps {
      
      static enum Fields {
        _id, stamp
      }
      
      final static String NAME = "timestamps";
      
      final static String CREATE_QUERY = "CREATE TABLE timestamps (" +
          "_id  TEXT PRIMARY KEY, " +
          "stamp DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL" +
          ");";
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
  
  interface CacheLifetime {
    boolean isExpired();
    void update();
  }
  
  class DbCacheLifetime implements CacheLifetime {
    
    private final String objId;
    private final TimeStamp TTL;
    
    DbCacheLifetime(String id, TimeStamp ttl) {
      this.objId = id;
      this.TTL = ttl;
    }
    
    @Override
    public boolean isExpired() {
      final SQLiteDatabase db = helper.getReadableDatabase();
      final String selection = Tables.Timestamps.Fields._id + " = '" + objId + "'";
      final String target = "strftime('%s', 'now') - strftime('%s', " + Tables.Timestamps.Fields.stamp + ")";
      final Cursor cursor = db.query(Tables.Timestamps.NAME, new String[] {target}, selection, 
          null, null, null, null, null);
      try {
        if (cursor.moveToFirst()) {
            final TimeStamp age = TimeStamp.createFrom(cursor.getInt(0), TimeUnit.SECONDS);
            return age.compareTo(TTL) > 0;
        }
      } finally {
        cursor.close();
      }
      return true;
    }
    
    @Override
    public void update() {
      final ContentValues values = new ContentValues();
      values.put(Tables.Timestamps.Fields._id.name(), objId);
      final SQLiteDatabase db = helper.getWritableDatabase();
      db.insertWithOnConflict(Tables.Timestamps.NAME, null/* nullColumnHack */, values,
          SQLiteDatabase.CONFLICT_REPLACE);
    }
  }
  
  //TODO: singleton
  class StubCacheLifetime implements CacheLifetime {
    
    private StubCacheLifetime() {}
    
    @Override
    public boolean isExpired() {
      return false;
    }
    
    @Override
    public void update() {}
  }
  
  final CacheLifetime getLifetime(String id, TimeStamp ttl) {
    return new DbCacheLifetime(id, ttl);
  }
  
  final CacheLifetime getStubLifetime() {
    return new StubCacheLifetime();
  }
  
  final void queryAuthors(String handleFilter, AuthorsVisitor visitor) {
    Log.d(TAG, "queryAuthors(filter=%s)", handleFilter);
    final String selection = handleFilter.equals("0-9")
      ? "SUBSTR(" + Tables.Authors.Fields.handle + ", 1, 1) NOT BETWEEN 'A' AND 'Z' COLLATE NOCASE"
      : Tables.Authors.Fields.handle + " LIKE '" + handleFilter + "%'";
    queryAuthorsInternal(selection, visitor);
  }
  
  final void queryAuthors(Country country, AuthorsVisitor visitor) {
    Log.d(TAG, "queryAuthors(country=%d)", country.id);
    final String idQuery = Tables.CountryAuthors.getAuthorsIdsSelection(country);
    final String selection = Tables.Authors.Fields._id + " IN (" + idQuery + ")";
    queryAuthorsInternal(selection, visitor);
  }
  
  final void queryAuthors(int id, AuthorsVisitor visitor) {
    Log.d(TAG, "queryAuthors(id=%d)", id);
    final String selection = Tables.Authors.Fields._id + " = " + id;
    queryAuthorsInternal(selection, visitor);
  }
  
  private void queryAuthorsInternal(String selection, AuthorsVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final Cursor cursor = db.query(Tables.Authors.NAME, null, selection, null, null, null, null);
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        visitor.setCountHint(count);
        while (cursor.moveToNext()) {
          visitor.accept(Tables.Authors.createAuthor(cursor));
        }
      }
    } finally {
      cursor.close();
    }
  }

  final void queryTracks(Author author, Catalog.TracksVisitor visitor) {
    Log.d(TAG, "queryTracks(author=%d)", author.id);
    final String idQuery = Tables.AuthorTracks.getTracksIdsSelection(author);
    final String selection = Tables.Tracks.Fields._id + " IN (" + idQuery + ")";
    queryTracksInternal(selection, visitor);
  }
  
  final void queryTracks(int id, Catalog.TracksVisitor visitor) {
    Log.d(TAG, "queryTracks(id=%d)", id);
    final String selection = Tables.Tracks.Fields._id + " = " + id;
    queryTracksInternal(selection, visitor);
  }
  
  private void queryTracksInternal(String selection, Catalog.TracksVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
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
  
  final void addCountryAuthor(Country country, Author author) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insert(Tables.CountryAuthors.NAME, null/* nullColumnHack */, Tables.CountryAuthors.createValues(country, author));
  }
  
  final void addAuthor(Author obj) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insertWithOnConflict(Tables.Authors.NAME, null/* nullColumnHack */, Tables.Authors.createValues(obj),
      SQLiteDatabase.CONFLICT_REPLACE);
  }
  
  final void addTrack(Track obj) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insertWithOnConflict(Tables.Tracks.NAME, null/* nullColumnHack */, Tables.Tracks.createValues(obj),
            SQLiteDatabase.CONFLICT_REPLACE);
  }

  final void addAuthorTrack(Author author, Track track) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insert(Tables.AuthorTracks.NAME, null/* nullColumnHack */, Tables.AuthorTracks.createValues(author, track));
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
      db.execSQL(Tables.CountryAuthors.getCreateQuery());
      db.execSQL(Tables.Authors.CREATE_QUERY);
      db.execSQL(Tables.Tracks.CREATE_QUERY);
      db.execSQL(Tables.AuthorTracks.getCreateQuery());
      db.execSQL(Tables.Timestamps.CREATE_QUERY);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
      Log.d(TAG, "Upgrading database %d -> %d", oldVersion, newVersion);
      if (oldVersion == 1) {
        final String[] tables = {
            Tables.CountryAuthors.NAME,
            Tables.Authors.NAME,
            Tables.Tracks.NAME,
            Tables.AuthorTracks.NAME,
            Tables.Timestamps.NAME
        };
        for (String table : tables) {
          db.execSQL(String.format(Tables.DROP_QUERY, table));
        }
        onCreate(db);
      }
    }
  }
}
