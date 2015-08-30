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

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteStatement;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.dbhelpers.Grouping;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;
import app.zxtune.fs.dbhelpers.Utils;

/**
 * Version 1 
 * CREATE TABLE authors (_id INTEGER PRIMARY KEY, nickname TEXT NOT NULL, name TEXT) 
 * CREATE TABLE parties (_id INTEGER PRIMARY KEY, name TEXT NOT NULL, year INTEGER NOT NULL) 
 * CREATE TABLE tracks (_id INTEGER PRIMARY KEY, filename TEXT NOT NULL, title TEXT, votes TEXT, 
 * duration TEXT, year INTEGER, partyplace INTEGER) 
 * CREATE TABLE {authors,parties}_tracks (hash INTEGER UNIQUE, group_id INTEGER, track_id INTEGER)
 * use hash as 1000000 * author +  * track to support multiple insertings of same pair
 * 
 * Version 2
 * CREATE TABLE tracks (_id INTEGER PRIMARY KEY, filename TEXT NOT NULL, title TEXT, votes TEXT, 
 * duration TEXT, year INTEGER, compo TEXT, partyplace INTEGER)
 * 
 * Version 3
 * 
 * Change format of {authors,parties}_tracks to grouping
 * Use timestamps
 * 
 */

final class Database {

  final static String TAG = Database.class.getName();

  final static String NAME = "www.zxart.ee";
  final static int VERSION = 3;

  final static class Tables {

    final static class Authors {

      static enum Fields {
        _id, nickname, name
      }

      final static String NAME = "authors";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
          + " INTEGER PRIMARY KEY, " + Fields.nickname + " TEXT NOT NULL, " + Fields.name
          + " TEXT);";

      final static String INSERT_STATEMENT =
          "REPLACE INTO authors VALUES (?, ?, ?);";
      
      private final SQLiteStatement insertStatement;
      
      Authors(SQLiteOpenHelper helper) {
        this.insertStatement = helper.getWritableDatabase().compileStatement(INSERT_STATEMENT);
      }
      
      final synchronized void add(Author obj) {
        insertStatement.bindLong(1 + Fields._id.ordinal(), obj.id);
        insertStatement.bindString(1 + Fields.nickname.ordinal(), obj.nickname);
        insertStatement.bindString(1 + Fields.name.ordinal(), obj.name);
        insertStatement.executeInsert();
      }

      private static Author createAuthor(Cursor cursor) {
        final int id = cursor.getInt(Tables.Authors.Fields._id.ordinal());
        final String name = cursor.getString(Tables.Authors.Fields.name.ordinal());
        final String nickname = cursor.getString(Tables.Authors.Fields.nickname.ordinal());
        return new Author(id, nickname, name);
      }

      static String getSelection(int id) {
        return Fields._id + " = " + id;
      }
    }

    final static class Parties {

      static enum Fields {
        _id, name, year
      }

      final static String NAME = "parties";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
          + " INTEGER PRIMARY KEY, " + Fields.name + " TEXT NOT NULL, " + Fields.year
          + " INTEGER NOT NULL);";

      final static String INSERT_STATEMENT =
          "REPLACE INTO parties VALUES (?, ?, ?);";
      
      private final SQLiteStatement insertStatement;
      
      Parties(SQLiteOpenHelper helper) {
        this.insertStatement = helper.getWritableDatabase().compileStatement(INSERT_STATEMENT);
      }
      
      final synchronized void add(Party obj) {
        insertStatement.bindLong(1 + Fields._id.ordinal(), obj.id);
        insertStatement.bindString(1 + Fields.name.ordinal(), obj.name);
        insertStatement.bindLong(1 + Fields.year.ordinal(), obj.year);
        insertStatement.executeInsert();
      }

      private static Party createParty(Cursor cursor) {
        final int id = cursor.getInt(Tables.Parties.Fields._id.ordinal());
        final String name = cursor.getString(Tables.Parties.Fields.name.ordinal());
        final int year = cursor.getInt(Tables.Parties.Fields.year.ordinal());
        return new Party(id, name, year);
      }

      static String getSelection(int id) {
        return Fields._id + " = " + id;
      }
    }

    final static class Tracks {

      static enum Fields {
        _id, filename, title, votes, duration, year, compo, partyplace
      }

      final static String NAME = "tracks";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
          + " INTEGER PRIMARY KEY, " + Fields.filename + " TEXT NOT NULL, " + Fields.title
          + " TEXT, " + Fields.votes + " TEXT, " + Fields.duration + " INTEGER, " + Fields.year
          + " INTEGER, " + Fields.compo + " TEXT, " + Fields.partyplace + " INTEGER);";
      
      final static String INSERT_STATEMENT =
          "REPLACE INTO tracks VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
      
      private final SQLiteStatement insertStatement;
      
      Tracks(SQLiteOpenHelper helper) {
        this.insertStatement = helper.getWritableDatabase().compileStatement(INSERT_STATEMENT);
      }
      
      final synchronized void add(Track obj) {
        insertStatement.bindLong(1 + Fields._id.ordinal(), obj.id);
        insertStatement.bindString(1 + Fields.filename.ordinal(), obj.filename);
        insertStatement.bindString(1 + Fields.title.ordinal(), obj.title);
        insertStatement.bindString(1 + Fields.votes.ordinal(), obj.votes);
        insertStatement.bindString(1 + Fields.duration.ordinal(), obj.duration);
        insertStatement.bindLong(1 + Fields.year.ordinal(), obj.year);
        insertStatement.bindString(1 + Fields.compo.ordinal(), obj.compo);
        insertStatement.bindLong(1 + Fields.partyplace.ordinal(), obj.partyplace);
        insertStatement.executeInsert();
      }
      
      static Track createTrack(Cursor cursor) {
        final int id = cursor.getInt(Tables.Tracks.Fields._id.ordinal());
        final String filename = cursor.getString(Tables.Tracks.Fields.filename.ordinal());
        final String title = cursor.getString(Tables.Tracks.Fields.title.ordinal());
        final String votes = cursor.getString(Tables.Tracks.Fields.votes.ordinal());
        final String duration = cursor.getString(Tables.Tracks.Fields.duration.ordinal());
        final int year = cursor.getInt(Tables.Tracks.Fields.year.ordinal());
        final String compo = cursor.getString(Tables.Tracks.Fields.compo.ordinal());
        final int partyplace = cursor.getInt(Tables.Tracks.Fields.partyplace.ordinal());
        return new Track(id, filename, title, votes, duration, year, compo, partyplace);
      }
      
      static String getSelection(int id) {
        return Fields._id + " = " + id;
      }
      
      static String getSelection(String subquery) {
        return Fields._id + " IN (" + subquery + ")";
      }
    }
    
    final static class AuthorsTracks extends Grouping {

      final static String NAME = "authors_tracks";
      final static String CREATE_QUERY = Grouping.createQuery(NAME);
      
      AuthorsTracks(SQLiteOpenHelper helper) {
        super(helper, NAME, 32);
      }

      final void add(Author author, Track track) {
        add(author.id, track.id);
      }
      
      final String getTracksIdsSelection(Author author) {
        return getIdsSelection(author.id);
      }
    }

    final static class PartiesTracks extends Grouping {

      final static String NAME = "parties_tracks";
      final static String CREATE_QUERY = Grouping.createQuery(NAME);
      
      PartiesTracks(SQLiteOpenHelper helper) {
        super(helper, NAME, 32);
      }

      final void add(Party party, Track track) {
        add(party.id, track.id);
      }
      
      final String getTracksIdsSelection(Party party) {
        return getIdsSelection(party.id);
      }
    }
  }

  private final Helper helper;
  private final Tables.Authors authors;
  private final Tables.AuthorsTracks authorsTracks;
  private final Tables.Parties parties;
  private final Tables.PartiesTracks partiesTracks;
  private final Tables.Tracks tracks;
  private final Timestamps timestamps;

  Database(Context context) {
    this.helper = Helper.create(context);
    this.authors = new Tables.Authors(helper);
    this.authorsTracks = new Tables.AuthorsTracks(helper);
    this.parties = new Tables.Parties(helper);
    this.partiesTracks = new Tables.PartiesTracks(helper);
    this.tracks = new Tables.Tracks(helper);
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
  
  final Timestamps.Lifetime getPartiesLifetime(TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Parties.NAME, ttl);
  }

  final Timestamps.Lifetime getPartyTracksLifetime(Party party, TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Parties.NAME + party.id, ttl);
  }
  
  final Timestamps.Lifetime getTopLifetime(TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Tracks.NAME, ttl);
  }
  
  final boolean queryAuthors(Integer id, Catalog.AuthorsVisitor visitor) {
    Log.d(TAG, "queryAuthors(id=%s)", id);
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = id != null ? Tables.Authors.getSelection(id) : null;
    final Cursor cursor = db.query(Tables.Authors.NAME, null, selection, null, null, null, null);
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        visitor.setCountHint(count);
        while (cursor.moveToNext()) {
          visitor.accept(Tables.Authors.createAuthor(cursor));
        }
        return true;
      }
    } finally {
      cursor.close();
    }
    return false;
  }

  final void addAuthor(Author obj) {
    authors.add(obj);
  }

  final boolean queryParties(Integer id, Catalog.PartiesVisitor visitor) {
    Log.d(TAG, "queryParties(id=%s)", id);
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = id != null ? Tables.Parties.getSelection(id) : null;
    final Cursor cursor = db.query(Tables.Parties.NAME, null, selection, null, null, null, null);
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        visitor.setCountHint(count);
        while (cursor.moveToNext()) {
          visitor.accept(Tables.Parties.createParty(cursor));
        }
        return true;
      }
    } finally {
      cursor.close();
    }
    return false;
  }

  final void addParty(Party obj) {
    parties.add(obj);
  }
  
  final boolean queryTrack(int id, Catalog.TracksVisitor visitor) {
    final String selection = Tables.Tracks.getSelection(id);
    return queryTracks(selection, visitor);
  }
  
  final boolean queryAuthorTracks(Author author, Catalog.TracksVisitor visitor) {
    final String selection = Tables.Tracks.getSelection(authorsTracks.getTracksIdsSelection(author));
    return queryTracks(selection, visitor);
  }

  final boolean queryPartyTracks(Party party, Catalog.TracksVisitor visitor) {
    final String selection = Tables.Tracks.getSelection(partiesTracks.getTracksIdsSelection(party));
    return queryTracks(selection, visitor);
  }

  private boolean queryTracks(String selection, Catalog.TracksVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final Cursor cursor = db.query(Tables.Tracks.NAME, null, selection, null, null, null, null);
    return queryTracks(cursor, visitor);
  }
  
  final boolean queryTopTracks(int limit, Catalog.TracksVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final Cursor cursor = db.query(Tables.Tracks.NAME, null, null, null, null, null, 
        Tables.Tracks.Fields.votes.name() + " DESC", Integer.toString(limit));
    return queryTracks(cursor, visitor);
  }
  
  private static boolean queryTracks(Cursor cursor, Catalog.TracksVisitor visitor) {
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
  
  final void addTrack(Track track) {
    tracks.add(track);
  }
  
  final void addAuthorTrack(Author author, Track track) {
    authorsTracks.add(author, track);
  }

  final void addPartyTrack(Party party, Track track) {
    partiesTracks.add(party, track);
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
