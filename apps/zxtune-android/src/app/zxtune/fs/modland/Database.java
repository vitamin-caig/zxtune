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
 * CREATE TABLE authors (_id INTEGER PRIMARY KEY, nickname TEXT NOT NULL, tracks INTEGER)
 * CREATE TABLE collections (_id INTEGER PRIMARY KEY, title TEXT NOT NULL, tracks INTEGER)
 * CREATE TABLE tracks (_id INTEGER PRIMARY KEY, path TEXT NOT NULL, size INTEGER)
 * CREATE TABLE authors_tracks (hash INTEGER UNIQUE, author INTEGER, track INTEGER)
 * CREATE TABLE collections_tracks (hash INTEGER UNIQUE, collection INTEGER, track INTEGER)
 *
 * use hash as 10000000000 * author/collection_id + track to support multiple insertings of same pair
 */

final class Database {

  final static String TAG = Database.class.getName();

  final static String NAME = "modland.com";
  final static int VERSION = 1;

  final static class Tables {

    final static String DROP_QUERY = "DROP TABLE ?;";

    final static class Authors {

      static enum Fields {
        _id, nickname, tracks
      }

      final static String NAME = "authors";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
              + " INTEGER PRIMARY KEY, " + Fields.nickname + " TEXT NOT NULL, " + Fields.tracks
              + " INTEGER);";
    }

    final static class Collections {

      static enum Fields {
        _id, title, tracks
      }

      final static String NAME = "collections";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
              + " INTEGER PRIMARY KEY, " + Fields.title + " TEXT NOT NULL, " + Fields.tracks
              + " INTEGER);";
    }

    final static class Tracks {

      static enum Fields {
        _id, path, size
      }

      final static String NAME = "tracks";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields._id
              + " INTEGER PRIMARY KEY, " + Fields.path + " TEXT NOT NULL, " + Fields.size + " INTEGER);";
    }

    final static class AuthorsTracks {

      static enum Fields {
        hash, author, track
      }

      final static String NAME = "authors_tracks";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields.hash
              + " INTEGER UNIQUE, " + Fields.author + " INTEGER, " + Fields.track + " INTEGER);";
    }

    final static class CollectionsTracks {

      static enum Fields {
        hash, collection, track
      }

      final static String NAME = "collections_tracks";

      final static String CREATE_QUERY = "CREATE TABLE " + NAME + " (" + Fields.hash
              + " INTEGER UNIQUE, " + Fields.collection + " INTEGER, " + Fields.track + " INTEGER);";
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

  final void queryAuthors(String filter, Catalog.AuthorsVisitor visitor) {
    Log.d(TAG, "queryAuthors(filter=" + filter + ")");
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = filter.equals("#")
      ? "SUBSTR(" + Tables.Authors.Fields.nickname + ", 1, 1) NOT BETWEEN 'A' AND 'Z'"
      : Tables.Authors.Fields.nickname + " LIKE '" + filter + "%'";
    final Cursor cursor = db.query(Tables.Authors.NAME, null, selection, null, null, null, null);
    try {
      while (cursor.moveToNext()) {
        visitor.accept(createAuthor(cursor));
      }
    } finally {
      cursor.close();
    }
  }

  final Author queryAuthor(int id) {
    Log.d(TAG, "queryAuthor(id=" + id + ")");
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = Tables.Authors.Fields._id + " = " + id;
    final Cursor cursor = db.query(Tables.Authors.NAME, null, selection, null, null, null, null);
    try {
      return cursor.moveToNext()
        ? createAuthor(cursor)
        : null;
    } finally {
      cursor.close();
    }
  }

  final void addAuthor(Author obj) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insertWithOnConflict(Tables.Authors.NAME, null/* nullColumnHack */, createValues(obj),
            SQLiteDatabase.CONFLICT_REPLACE);
  }

  final void queryAuthorTracks(int authorId, Catalog.TracksVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final String selection = createAuthorTracksSelection(authorId);
    final Cursor cursor = db.query(Tables.Tracks.NAME, null, selection, null, null, null, null);
    try {
      while (cursor.moveToNext()) {
        visitor.accept(createTrack(cursor));
      }
    } finally {
      cursor.close();
    }
  }

  private static String createSingleTrackSelection(int id) {
    return Tables.Tracks.Fields._id + " = " + id;
  }

  private static String createAuthorTracksSelection(int author) {
    final String idQuery =
            SQLiteQueryBuilder.buildQueryString(true, Tables.AuthorsTracks.NAME,
                    new String[]{Tables.AuthorsTracks.Fields.track.name()}, Tables.AuthorsTracks.Fields.author + " = "
                    + author, null, null, null, null);
    return Tables.Tracks.Fields._id + " IN (" + idQuery + ")";
  }

  final void addTrack(Track obj) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insertWithOnConflict(Tables.Tracks.NAME, null/* nullColumnHack */, createValues(obj),
            SQLiteDatabase.CONFLICT_REPLACE);
  }

  final void addAuthorsTrack(Track obj, int authorId) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insert(Tables.AuthorsTracks.NAME, null/* nullColumnHack */, createAuthorTrackValues(authorId, obj.id));
  }

  private static Author createAuthor(Cursor cursor) {
    final int id = cursor.getInt(Tables.Authors.Fields._id.ordinal());
    final String nickname = cursor.getString(Tables.Authors.Fields.nickname.ordinal());
    final int tracks = cursor.getInt(Tables.Authors.Fields.tracks.ordinal());
    return new Author(id, nickname, tracks);
  }

  private static ContentValues createValues(Author obj) {
    final ContentValues res = new ContentValues();
    res.put(Tables.Authors.Fields._id.name(), obj.id);
    res.put(Tables.Authors.Fields.nickname.name(), obj.nickname);
    res.put(Tables.Authors.Fields.tracks.name(), obj.tracks);
    return res;
  }

  private static Track createTrack(Cursor cursor) {
    final int id = cursor.getInt(Tables.Tracks.Fields._id.ordinal());
    final String path = cursor.getString(Tables.Tracks.Fields.path.ordinal());
    final int size = cursor.getInt(Tables.Tracks.Fields.size.ordinal());
    return new Track(id, path, size);
  }

  private static ContentValues createValues(Track obj) {
    final ContentValues res = new ContentValues();
    res.put(Tables.Tracks.Fields._id.name(), obj.id);
    res.put(Tables.Tracks.Fields.path.name(), obj.path);
    res.put(Tables.Tracks.Fields.size.name(), obj.size);
    return res;
  }

  private static ContentValues createAuthorTrackValues(int author, long track) {
    final long hash = 10000000000l * author + track;
    final ContentValues res = new ContentValues();
    res.put(Tables.AuthorsTracks.Fields.hash.name(), hash);
    res.put(Tables.AuthorsTracks.Fields.author.name(), author);
    res.put(Tables.AuthorsTracks.Fields.track.name(), track);
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
      db.execSQL(Tables.Collections.CREATE_QUERY);
      db.execSQL(Tables.Tracks.CREATE_QUERY);
      db.execSQL(Tables.AuthorsTracks.CREATE_QUERY);
      db.execSQL(Tables.CollectionsTracks.CREATE_QUERY);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
      Log.d(TAG, String.format("Upgrading database %d -> %d", oldVersion, newVersion));
      db.execSQL(Tables.DROP_QUERY, new Object[] {Tables.Authors.NAME});
      db.execSQL(Tables.DROP_QUERY, new Object[] {Tables.Collections.NAME});
      db.execSQL(Tables.DROP_QUERY, new Object[] {Tables.Tracks.NAME});
      db.execSQL(Tables.DROP_QUERY, new Object[] {Tables.AuthorsTracks.NAME});
      db.execSQL(Tables.DROP_QUERY, new Object[] {Tables.CollectionsTracks.NAME});
      onCreate(db);
    }
  }
}
