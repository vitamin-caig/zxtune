/**
 *
 * @file
 *
 * @brief DAL helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs.modarchive;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import java.nio.ByteBuffer;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.VfsCache;
import app.zxtune.fs.dbhelpers.DBProvider;
import app.zxtune.fs.dbhelpers.Grouping;
import app.zxtune.fs.dbhelpers.Objects;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;
import app.zxtune.fs.dbhelpers.Utils;

/**
 * Version 1
 *
 * CREATE TABLE authors (_id INTEGER PRIMARY KEY, alias TEXT NOT NULL)
 * CREATE TABLE genres (_id INTEGER PRIMARY KEY, name TEXT NOT NULL, tracks INTEGER NOT NULL)
 * CREATE TABLE tracks (_id INTEGER PRIMARY KEY, filename TEXT NOT NULL, title TEXT NOT NULL, size INTEGER NOT NULL)
 * 
 * author_tracks and genre_authors are 32-bit grouping
 * 
 * Standard timestamps table(s) 
 * 
 */

final class Database {

  final static String TAG = Database.class.getName();

  final static String NAME = "modarchive.org";
  final static int VERSION = 1;

  final static class Tables {

    final static class Authors extends Objects {

      static enum Fields {
        _id, alias
      }
      
      final static String NAME = "authors";
      
      final static String CREATE_QUERY = "CREATE TABLE authors (" +
        "_id INTEGER PRIMARY KEY, " +
        "alias TEXT NOT NULL" +
        ");";
      ;
      
      Authors(DBProvider helper) {
        super(helper, NAME, Fields.values().length);
      }
      
      final void add(Author obj) {
        add(obj.id, obj.alias);
      }

      static Author createAuthor(Cursor cursor) {
        final int id = cursor.getInt(Fields._id.ordinal());
        final String alias = cursor.getString(Fields.alias.ordinal());
        return new Author(id, alias);
      }

      static String getSelection(String subquery) {
        return Fields._id + " IN (" + subquery + ")";
      }
    }

    final static class Tracks extends Objects {

      static enum Fields {
        _id, filename, title, size
      }

      final static String NAME = "tracks";

      final static String CREATE_QUERY = "CREATE TABLE tracks (" +
          "_id INTEGER PRIMARY KEY, " +
          "filename TEXT NOT NULL, " +
          "title TEXT NOT NULL, " +
          "size INTEGER NOT NULL" +
          ");";

      Tracks(DBProvider helper) {
        super(helper, NAME, Fields.values().length);
      }
      
      final void add(Track obj) {
        add(obj.id, obj.filename, obj.title, obj.size);
      }
      
      static Track createTrack(Cursor cursor) {
        return createTrack(cursor, 0);
      }
      
      static Track createTrack(Cursor cursor, int fieldOffset) {
        final int id = cursor.getInt(fieldOffset + Fields._id.ordinal());
        final String filename = cursor.getString(fieldOffset + Fields.filename.ordinal());
        final String title = cursor.getString(fieldOffset + Fields.title.ordinal());
        final int size = cursor.getInt(fieldOffset + Fields.size.ordinal());
        return new Track(id, filename, title, size);
      }

      static String getSelection(String subquery) {
        return Fields._id + " IN (" + subquery + ")";
      }
    }
    
    final static class AuthorTracks extends Grouping {
      
      final static String NAME = "author_tracks";
      final static String CREATE_QUERY = Grouping.createQuery(NAME);
      
      AuthorTracks(DBProvider helper) {
        super(helper, NAME, 32);
      }
      
      final void add(Author author, Track track) {
        add(author.id, track.id);
      }
      
      final String getTracksIdsSelection(Author author) {
        return getIdsSelection(author.id);
      }
    }

    final static class Genres extends Objects {
      
      static enum Fields {
        _id, name, tracks
      }
      
      final static String NAME = "genres";
      
      final static String CREATE_QUERY = "CREATE TABLE genres (" +
        "_id INTEGER PRIMARY KEY, " +
        "name TEXT NOT NULL, " +
        "tracks INTEGER NOT NULL" +
        ");";
      ;

      Genres(DBProvider helper) {
        super(helper, NAME, Fields.values().length);
      }
      
      final void add(Genre obj) {
        add(obj.id, obj.name, obj.tracks);
      }
            
      static Genre createGenre(Cursor cursor) {
        final int id = cursor.getInt(Fields._id.ordinal());
        final String name = cursor.getString(Fields.name.ordinal());
        final int tracks = cursor.getInt(Fields.tracks.ordinal());
        return new Genre(id, name, tracks);
      }
    }

    final static class GenreTracks extends Grouping {

      final static String NAME = "genre_tracks";
      final static String CREATE_QUERY = Grouping.createQuery(NAME);
      
      GenreTracks(DBProvider helper) {
        super(helper, NAME, 32);
      }
      
      final void add(Genre genre, Track track) {
        add(genre.id, track.id);
      }
      
      final String getTracksIdsSelection(Genre genre) {
        return getIdsSelection(genre.id);
      }
    }
  }

  private final DBProvider helper;
  private final Tables.Authors authors;
  private final Tables.AuthorTracks authorTracks;
  private final Tables.Genres genres;
  private final Tables.GenreTracks genreTracks;
  private final Tables.Tracks tracks;
  private final Timestamps timestamps;
  private final String findQuery;
  private final VfsCache cacheDir;

  Database(Context context, VfsCache cache) {
    this.helper = new DBProvider(Helper.create(context));
    this.authors = new Tables.Authors(helper);
    this.authorTracks = new Tables.AuthorTracks(helper);
    this.genres = new Tables.Genres(helper);
    this.genreTracks = new Tables.GenreTracks(helper);
    this.tracks = new Tables.Tracks(helper);
    this.timestamps = new Timestamps(helper);
    this.findQuery = "SELECT * " +
        "FROM authors LEFT OUTER JOIN tracks ON " +
        "tracks." + Tables.Tracks.getSelection(authorTracks.getIdsSelection("authors._id")) +
        " WHERE tracks.filename || tracks.title LIKE '%' || ? || '%'";
    this.cacheDir = cache.createNested("modarchive.org");
  }

  final Transaction startTransaction() {
    return new Transaction(helper.getWritableDatabase());
  }
  
  final Timestamps.Lifetime getAuthorsLifetime(TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Authors.NAME, ttl);
  }
  
  final Timestamps.Lifetime getGenresLifetime(TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Genres.NAME, ttl);
  }

  final Timestamps.Lifetime getAuthorTracksLifetime(Author author, TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Authors.NAME + author.id, ttl);
  }
  
  final Timestamps.Lifetime getGenreTracksLifetime(Genre genre, TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Genres.NAME + genre.id, ttl);
  }
  
  final boolean queryAuthors(Catalog.AuthorsVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final Cursor cursor = db.query(Tables.Authors.NAME, null, null, null, null, null, null);
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
  
  final boolean queryGenres(Catalog.GenresVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final Cursor cursor = db.query(Tables.Genres.NAME, null, null, null, null, null, null);
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        visitor.setCountHint(count);
        while (cursor.moveToNext()) {
          visitor.accept(Tables.Genres.createGenre(cursor));
        }
        return true;
      }
    } finally {
      cursor.close();
    }
    return false;
  }
  
  final void addGenre(Genre obj) {
    genres.add(obj);
  }
  
  final boolean queryTracks(Author author, Catalog.TracksVisitor visitor) {
    final String selection = Tables.Tracks.getSelection(authorTracks.getTracksIdsSelection(author));
    return queryTracksInternal(selection, visitor);
  }

  final boolean queryTracks(Genre genre, Catalog.TracksVisitor visitor) {
    final String selection = Tables.Tracks.getSelection(genreTracks.getTracksIdsSelection(genre));
    return queryTracksInternal(selection, visitor);
  }
  
  private boolean queryTracksInternal(String selection, Catalog.TracksVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
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
  
  final synchronized void findTracks(String query, Catalog.FoundTracksVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final Cursor cursor = db.rawQuery(findQuery, new String[]{query});
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        visitor.setCountHint(count);
        while (cursor.moveToNext()) {
          final Author author = Tables.Authors.createAuthor(cursor);
          final Track track = Tables.Tracks.createTrack(cursor, Tables.Authors.Fields.values().length);
          visitor.accept(author, track);
        }
      }
    } finally {
      cursor.close();
    }
  }

  final void addTrack(Track obj) {
    tracks.add(obj);
  }

  final void addAuthorTrack(Author author, Track track) {
    authorTracks.add(author, track);
  }
  
  final void addGenreTrack(Genre genre, Track track) {
    genreTracks.add(genre, track);
  }

  final ByteBuffer getTrackContent(int id) {
    final String filename = Integer.toString(id);
    return cacheDir.getCachedFileContent(filename);
  }

  final void addTrackContent(int id, ByteBuffer content) {
    final String filename = Integer.toString(id);
    cacheDir.putCachedFileContent(filename, content);
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
      db.execSQL(Tables.AuthorTracks.CREATE_QUERY);
      db.execSQL(Tables.Genres.CREATE_QUERY);
      db.execSQL(Tables.GenreTracks.CREATE_QUERY);
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
