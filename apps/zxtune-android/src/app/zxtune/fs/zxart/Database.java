/**
 *
 * @file
 *
 * @brief DAL helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.zxart;

import java.util.concurrent.TimeUnit;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.util.Log;
import app.zxtune.TimeStamp;

/**
 * Version 1 
 * CREATE TABLE authors (_id INTEGER PRIMARY KEY, nickname TEXT NOT NULL, name TEXT) 
 * CREATE TABLE parties (_id INTEGER PRIMARY KEY, name TEXT NOT NULL, year INTEGER NOT NULL) 
 * CREATE TABLE tracks (_id INTEGER PRIMARY KEY, filename TEXT NOT NULL, title TEXT, votes TEXT, 
 * duration TEXT, year INTEGER, partyplace INTEGER) 
 * CREATE TABLE {authors,parties}_tracks (hash INTEGER UNIQUE, group_id INTEGER, track_id INTEGER)
 * use hash as 1000000 * author +  * track to support multiple insertings of same pair
 */

final class Database {

  final static String TAG = Database.class.getName();

  final static String NAME = "www.zxart.ee";
  final static int VERSION = 1;

  final static class Tables {

    final static String DROP_QUERY = "DROP TABLE ?;";

    final static class Authors {

      static enum Fields {
        _id, nickname, name
      }

      final static String NAME = "authors";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
          + " INTEGER PRIMARY KEY, " + Fields.nickname + " TEXT NOT NULL, " + Fields.name
          + " TEXT);";
    }

    final static class Parties {

      static enum Fields {
        _id, name, year
      }

      final static String NAME = "parties";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
          + " INTEGER PRIMARY KEY, " + Fields.name + " TEXT NOT NULL, " + Fields.year
          + " INTEGER NOT NULL);";
    }

    final static class Tracks {

      static enum Fields {
        _id, filename, title, votes, duration, year, partyplace
      }

      final static String NAME = "tracks";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
          + " INTEGER PRIMARY KEY, " + Fields.filename + " TEXT NOT NULL, " + Fields.title
          + " TEXT, " + Fields.votes + " TEXT, " + Fields.duration + " INTEGER, " + Fields.year
          + " INTEGER, " + Fields.partyplace + " INTEGER);";
    }
    
    final static class Grouping {
      
      static enum Fields {
        hash, group_id, track_id
      }
      
      final static String makeCreateQuery(String name) {
        return "CREATE TABLE " + name + " (" + Fields.hash
            + " INTEGER UNIQUE, " + Fields.group_id + " INTEGER, " + Fields.track_id + " INTEGER);";
      }
    }

    final static class AuthorsTracks {

      final static String NAME = "authors_tracks";

      final static String CREATE_QUERY = Grouping.makeCreateQuery(NAME); 
    }

    final static class PartiesTracks {

      final static String NAME = "parties_tracks";

      final static String CREATE_QUERY = Grouping.makeCreateQuery(NAME);
    }
    
    final static class Timestamps {
      
      static enum Fields {
        _id, stamp
      }
      
      final static String NAME = "timestamps";
      
      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " ("
          + Fields._id + " TEXT PRIMARY KEY, "
          + Fields.stamp + " DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL);";
    }
  }

  private final Helper helper;

  Database(Context context) {
    this.helper = Helper.create(context);
  }

  static class Transaction {

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
  
  class CacheLifetime {
    
    private final String objId;
    
    CacheLifetime(String type, Integer id) {
      this.objId = id != null ? String.format("%s/%d", type, id) : type;
    }
    
    final boolean isExpired(TimeStamp ttl) {
      final SQLiteDatabase db = helper.getReadableDatabase();
      final String selection = Tables.Timestamps.Fields._id + " = '" + objId + "'";
      final String target = "strftime('%s', 'now') - strftime('%s', " + Tables.Timestamps.Fields.stamp + ")";
      final Cursor cursor = db.query(Tables.Timestamps.NAME, new String[] {target}, selection, 
          null, null, null, null, null);
      try {
        if (cursor.moveToFirst()) {
            final TimeStamp age = TimeStamp.createFrom(cursor.getInt(0), TimeUnit.SECONDS);
            return age.compareTo(ttl) > 0;
        }
      } finally {
        cursor.close();
      }
      return true;
    }
    
    final void update() {
      final ContentValues values = new ContentValues();
      values.put(Tables.Timestamps.Fields._id.name(), objId);
      final SQLiteDatabase db = helper.getWritableDatabase();
      db.insertWithOnConflict(Tables.Timestamps.NAME, null/* nullColumnHack */, values,
          SQLiteDatabase.CONFLICT_REPLACE);
    }
  }

  final Transaction startTransaction() {
    return new Transaction(helper.getWritableDatabase());
  }
  
  final boolean queryAuthors(Catalog.AuthorsVisitor visitor, Integer id) {
    Log.d(TAG, "queryAuthors(" + id + ")");
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = id != null ? Tables.Authors.Fields._id + " = " + id : null;
    final Cursor cursor = db.query(Tables.Authors.NAME, null, selection, null, null, null, null);
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        Log.d(TAG, count + " found");
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

  final boolean queryParties(Catalog.PartiesVisitor visitor, Integer id) {
    Log.d(TAG, "queryParties(" + id + ")");
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = id != null ? Tables.Parties.Fields._id + " = " + id : null;
    final Cursor cursor = db.query(Tables.Parties.NAME, null, selection, null, null, null, null);
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        Log.d(TAG, count + " found");
        visitor.setCountHint(count);
        while (cursor.moveToNext()) {
          visitor.accept(createParty(cursor));
        }
        return true;
      }
    } finally {
      cursor.close();
    }
    return false;
  }

  final void addParty(Party obj) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insertWithOnConflict(Tables.Parties.NAME, null/* nullColumnHack */, createValues(obj),
        SQLiteDatabase.CONFLICT_REPLACE);
  }
  
  final boolean queryAuthorTracks(Catalog.TracksVisitor visitor, Integer id, Integer author) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection =
        id != null ? createSingleTrackSelection(id) : createAuthorTracksSelection(author);
    final Cursor cursor = db.query(Tables.Tracks.NAME, null, selection, null, null, null, null);
    return queryTracks(cursor, visitor);
  }

  final boolean queryPartyTracks(Catalog.TracksVisitor visitor, Integer id, Integer party) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection =
        id != null ? createSingleTrackSelection(id) : createPartyTracksSelection(party);
    final Cursor cursor = db.query(Tables.Tracks.NAME, null, selection, null, null, null, null);
    return queryTracks(cursor, visitor);
  }
  
  final boolean queryTopTracks(Catalog.TracksVisitor visitor, Integer id, int limit) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = id != null ? createSingleTrackSelection(id) : null;
    final Cursor cursor = db.query(Tables.Tracks.NAME, null, selection, null, null, null, 
        Tables.Tracks.Fields.votes.name() + " DESC", Integer.toString(limit));
    return queryTracks(cursor, visitor);
  }
  
  private static boolean queryTracks(Cursor cursor, Catalog.TracksVisitor visitor) {
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        Log.d(TAG, count + " found");
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
  
  private static String createSingleTrackSelection(int id) {
    return Tables.Tracks.Fields._id + " = " + id;
  }

  private static String createAuthorTracksSelection(int author) {
    return createTracksSelection(Tables.AuthorsTracks.NAME, author);
  }

  private static String createPartyTracksSelection(int author) {
    return createTracksSelection(Tables.PartiesTracks.NAME, author);
  }
  
  private static String createTracksSelection(String tableName, int group) {
    final String idQuery =
        SQLiteQueryBuilder.buildQueryString(true, tableName,
            new String[] {
              Tables.Grouping.Fields.track_id.name()
            }, Tables.Grouping.Fields.group_id + " = "
                + group, null, null, null, null);
    return Tables.Tracks.Fields._id + " IN (" + idQuery + ")";
  }

  final void addTrack(Track obj, Integer author, Integer party) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insertWithOnConflict(Tables.Tracks.NAME, null/* nullColumnHack */, createValues(obj),
        SQLiteDatabase.CONFLICT_REPLACE);
    if (author != null) {
      db.insertWithOnConflict(Tables.AuthorsTracks.NAME, null/* nullColumnHack */, createValues(author, obj.id),
          SQLiteDatabase.CONFLICT_REPLACE);
    }
    if (party != null) {
      db.insertWithOnConflict(Tables.PartiesTracks.NAME, null/* nullColumnHack */, createValues(party, obj.id),
          SQLiteDatabase.CONFLICT_REPLACE);
    }
  }
  
  final CacheLifetime getAuthorsLifetime(Integer id) {
    return new CacheLifetime(Tables.Authors.NAME, id);
  }
  
  final CacheLifetime getPartiesLifetime(Integer id) {
    return new CacheLifetime(Tables.Parties.NAME, id);
  }
  
  final CacheLifetime getTopLifetime() {
    return new CacheLifetime(Tables.Tracks.NAME, null);
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

  private static Party createParty(Cursor cursor) {
    final int id = cursor.getInt(Tables.Parties.Fields._id.ordinal());
    final String name = cursor.getString(Tables.Parties.Fields.name.ordinal());
    final int year = cursor.getInt(Tables.Parties.Fields.year.ordinal());
    return new Party(id, name, year);
  }

  private static ContentValues createValues(Party obj) {
    final ContentValues res = new ContentValues();
    res.put(Tables.Parties.Fields._id.name(), obj.id);
    res.put(Tables.Parties.Fields.name.name(), obj.name);
    res.put(Tables.Parties.Fields.year.name(), obj.year);
    return res;
  }
  
  private static Track createTrack(Cursor cursor) {
    final int id = cursor.getInt(Tables.Tracks.Fields._id.ordinal());
    final String filename = cursor.getString(Tables.Tracks.Fields.filename.ordinal());
    final String title = cursor.getString(Tables.Tracks.Fields.title.ordinal());
    final String votes = cursor.getString(Tables.Tracks.Fields.votes.ordinal());
    final String duration = cursor.getString(Tables.Tracks.Fields.duration.ordinal());
    final int year = cursor.getInt(Tables.Tracks.Fields.year.ordinal());
    final int partyplace = cursor.getInt(Tables.Tracks.Fields.partyplace.ordinal());
    return new Track(id, filename, title, votes, duration, year, partyplace);
  }

  private static ContentValues createValues(Track obj) {
    final ContentValues res = new ContentValues();
    res.put(Tables.Tracks.Fields._id.name(), obj.id);
    res.put(Tables.Tracks.Fields.filename.name(), obj.filename);
    res.put(Tables.Tracks.Fields.title.name(), obj.title);
    res.put(Tables.Tracks.Fields.votes.name(), obj.votes);
    res.put(Tables.Tracks.Fields.duration.name(), obj.duration);
    res.put(Tables.Tracks.Fields.year.name(), obj.year);
    res.put(Tables.Tracks.Fields.partyplace.name(), obj.partyplace);
    return res;
  }

  private static ContentValues createValues(int group, int track) {
    final int hash = 1000000 * group + track;
    final ContentValues res = new ContentValues();
    res.put(Tables.Grouping.Fields.hash.name(), hash);
    res.put(Tables.Grouping.Fields.group_id.name(), group);
    res.put(Tables.Grouping.Fields.track_id.name(), track);
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
      db.execSQL(Tables.Parties.CREATE_QUERY);
      db.execSQL(Tables.Tracks.CREATE_QUERY);
      db.execSQL(Tables.AuthorsTracks.CREATE_QUERY);
      db.execSQL(Tables.PartiesTracks.CREATE_QUERY);
      db.execSQL(Tables.Timestamps.CREATE_QUERY);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
      Log.d(TAG, String.format("Upgrading database %d -> %d", oldVersion, newVersion));
      final String ALL_TABLES[] = {
          Tables.Authors.NAME, Tables.Parties.NAME, Tables.Tracks.NAME,
          Tables.AuthorsTracks.NAME, Tables.PartiesTracks.NAME,
          Tables.Timestamps.NAME
      };
      for (String table : ALL_TABLES) {
        db.execSQL(Tables.DROP_QUERY, new Object[] {
          table
        });
      }
      onCreate(db);
    }
  }
}
