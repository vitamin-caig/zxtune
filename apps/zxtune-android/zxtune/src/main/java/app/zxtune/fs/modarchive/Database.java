/**
 * @file
 * @brief DAL helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modarchive;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import androidx.annotation.Nullable;

import java.io.IOException;

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

  private static final String TAG = Database.class.getName();

  private static final String NAME = "modarchive.org";
  private static final int VERSION = 1;

  static final class Tables {

    static final class Authors extends Objects {

      enum Fields {
        _id, alias
      }

      static final String NAME = "authors";

      static final String CREATE_QUERY = "CREATE TABLE authors (" +
              "_id INTEGER PRIMARY KEY, " +
              "alias TEXT NOT NULL" +
              ");";

      Authors(DBProvider helper) {
        super(helper, NAME, Fields.values().length);
      }

      final void add(Author obj) {
        add(obj.getId(), obj.getAlias());
      }

      static Author createAuthor(Cursor cursor) {
        final int id = cursor.getInt(Fields._id.ordinal());
        final String alias = cursor.getString(Fields.alias.ordinal());
        return new Author(id, alias);
      }
    }

    static final class Tracks extends Objects {

      enum Fields {
        _id, filename, title, size
      }

      static final String NAME = "tracks";

      static final String CREATE_QUERY = "CREATE TABLE tracks (" +
              "_id INTEGER PRIMARY KEY, " +
              "filename TEXT NOT NULL, " +
              "title TEXT NOT NULL, " +
              "size INTEGER NOT NULL" +
              ");";

      Tracks(DBProvider helper) {
        super(helper, NAME, Fields.values().length);
      }

      final void add(Track obj) {
        add(obj.getId(), obj.getFilename(), obj.getTitle(), obj.getSize());
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

    static final class AuthorTracks extends Grouping {

      static final String NAME = "author_tracks";
      static final String CREATE_QUERY = Grouping.createQuery(NAME);

      AuthorTracks(DBProvider helper) {
        super(helper, NAME, 32);
      }

      final void add(Author author, Track track) {
        add(author.getId(), track.getId());
      }

      final String getTracksIdsSelection(Author author) {
        return getIdsSelection(author.getId());
      }
    }

    static final class Genres extends Objects {

      enum Fields {
        _id, name, tracks
      }

      static final String NAME = "genres";

      static final String CREATE_QUERY = "CREATE TABLE genres (" +
              "_id INTEGER PRIMARY KEY, " +
              "name TEXT NOT NULL, " +
              "tracks INTEGER NOT NULL" +
              ");";

      Genres(DBProvider helper) {
        super(helper, NAME, Fields.values().length);
      }

      final void add(Genre obj) {
        add(obj.getId(), obj.getName(), obj.getTracks());
      }

      static Genre createGenre(Cursor cursor) {
        final int id = cursor.getInt(Fields._id.ordinal());
        final String name = cursor.getString(Fields.name.ordinal());
        final int tracks = cursor.getInt(Fields.tracks.ordinal());
        return new Genre(id, name, tracks);
      }
    }

    static final class GenreTracks extends Grouping {

      static final String NAME = "genre_tracks";
      static final String CREATE_QUERY = Grouping.createQuery(NAME);

      GenreTracks(DBProvider helper) {
        super(helper, NAME, 32);
      }

      final void add(Genre genre, Track track) {
        add(genre.getId(), track.getId());
      }

      final String getTracksIdsSelection(Genre genre) {
        return getIdsSelection(genre.getId());
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

  Database(Context context) {
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
  }

  final void runInTransaction(Utils.ThrowingRunnable cmd) throws IOException {
    Utils.runInTransaction(helper, cmd);
  }

  final Timestamps.Lifetime getAuthorsLifetime(TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Authors.NAME, ttl);
  }

  final Timestamps.Lifetime getGenresLifetime(TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Genres.NAME, ttl);
  }

  final Timestamps.Lifetime getAuthorTracksLifetime(Author author, TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Authors.NAME + author.getId(), ttl);
  }

  final Timestamps.Lifetime getGenreTracksLifetime(Genre genre, TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Genres.NAME + genre.getId(), ttl);
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

  final void queryRandomTracks(Catalog.TracksVisitor visitor) {
    queryTracksInternal(null, "RANDOM() LIMIT 100", visitor);
  }

  private boolean queryTracksInternal(@Nullable String selection, Catalog.TracksVisitor visitor) {
    return queryTracksInternal(selection, null, visitor);
  }

  private boolean queryTracksInternal(@Nullable String selection, @Nullable String order,
                                      Catalog.TracksVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final Cursor cursor = db.query(Tables.Tracks.NAME, null, selection, null, null, null, order);
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
