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

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.amp.Catalog.AuthorsVisitor;
import app.zxtune.fs.dbhelpers.Grouping;
import app.zxtune.fs.dbhelpers.Timestamps;
import app.zxtune.fs.dbhelpers.Transaction;
import app.zxtune.fs.dbhelpers.Utils;

/**
 * Version 1
 *
 * CREATE TABLE authors (_id INTEGER PRIMARY KEY, handle TEXT NOT NULL, real_name TEXT NOT NULL)
 * CREATE TABLE tracks (_id INTEGER PRIMARY KEY, filename TEXT NOT NULL, size INTEGER NOT NULL)
 * 
 * author_tracks and country_authors are 32-bit grouping
 * 
 * Standard timestamps table(s) 
 * 
 * Version 2
 * 
 * CREATE TABLE groups (_id INTEGER PRIMARY KEY, name TEXT NOT NULL)
 * 
 */

final class Database {

  final static String TAG = Database.class.getName();

  final static String NAME = "amp.dascene.net";
  final static int VERSION = 2;

  final static class Tables {

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
    
    final static class AuthorTracks {
      
      final static String NAME = "author_tracks";
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

    final static class CountryAuthors {

      final static String NAME = "country_authors";
      private final static Grouping grouping = new Grouping(NAME, 32);
      
      static String createQuery() {
        return grouping.createQuery();
      }
      
      static ContentValues createValues(Country country, Author author) {
        return grouping.createValues(country.id, author.id);
      }
      
      static String getAuthorsIdsSelection(Country country) {
        return grouping.getIdsSelection(country.id);
      }
    }
    
    final static class Groups {
      
      static enum Fields {
        _id, name
      }
      
      final static String NAME = "groups";
      
      final static String CREATE_QUERY = "CREATE TABLE groups (" +
        "_id INTEGER PRIMARY KEY, " +
        "name TEXT NOT NULL" +
        ");";
      ;

      static ContentValues createValues(Group obj) {
        final ContentValues res = new ContentValues();
        res.put(Fields._id.name(), obj.id);
        res.put(Fields.name.name(), obj.name);
        return res;
      }
      
      static Group createGroup(Cursor cursor) {
        final int id = cursor.getInt(Fields._id.ordinal());
        final String name = cursor.getString(Fields.name.ordinal());
        return new Group(id, name);
      }
    }

    final static class GroupAuthors {

      final static String NAME = "group_authors";
      private final static Grouping grouping = new Grouping(NAME, 32);
      
      static String createQuery() {
        return grouping.createQuery();
      }
      
      static ContentValues createValues(Group group, Author author) {
        return grouping.createValues(group.id, author.id);
      }
      
      static String getAuthorsIdsSelection(Group group) {
        return grouping.getIdsSelection(group.id);
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
  
  final Timestamps.Lifetime getAuthorsLifetime(String handleFilter, TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Authors.NAME + handleFilter, ttl);
  }
  
  final Timestamps.Lifetime getCountryLifetime(Country country, TimeStamp ttl) {
    return timestamps.getLifetime("countries" + country.id, ttl);
  }

  final Timestamps.Lifetime getAuthorTracksLifetime(Author author, TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Authors.NAME + author.id, ttl);
  }
  
  final Timestamps.Lifetime getGroupsLifetime(TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Groups.NAME, ttl);
  }
  
  final Timestamps.Lifetime getGroupLifetime(Group group, TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Groups.NAME + group.id, ttl);
  }
  
  final Timestamps.Lifetime getStubLifetime() {
    return timestamps.getStubLifetime();
  }
  
  final void queryAuthors(String handleFilter, AuthorsVisitor visitor) {
    Log.d(TAG, "queryAuthors(filter=%s)", handleFilter);
    final String selection = handleFilter.equals(Catalog.NON_LETTER_FILTER)
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

  final void queryAuthors(Group group, AuthorsVisitor visitor) {
    Log.d(TAG, "queryAuthors(group=%d)", group.id);
    final String idQuery = Tables.GroupAuthors.getAuthorsIdsSelection(group);
    final String selection = Tables.Authors.Fields._id + " IN (" + idQuery + ")";
    queryAuthorsInternal(selection, visitor);
  }
  
  final Author queryAuthor(int id) {
    Log.d(TAG, "queryAuthors(id=%d)", id);
    final String selection = Tables.Authors.Fields._id + " = " + id;
    final SQLiteDatabase db = helper.getReadableDatabase();
    final Cursor cursor = db.query(Tables.Authors.NAME, null, selection, null, null, null, null);
    try {
      if (cursor.moveToNext()) {
        return Tables.Authors.createAuthor(cursor);
      }
    } finally {
      cursor.close();
    }
    return null;
  }
  
  //result may be empty, so don't treat it as error
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
  
  final void queryGroups(Catalog.GroupsVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final Cursor cursor = db.query(Tables.Groups.NAME, null, null, null, null, null, null);
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        visitor.setCountHint(count);
        while (cursor.moveToNext()) {
          visitor.accept(Tables.Groups.createGroup(cursor));
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

  final void addGroup(Group group) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insert(Tables.Groups.NAME, null/* nullColumnHack */, Tables.Groups.createValues(group));
  }
  
  final void addGroupAuthor(Group group, Author author) {
    final SQLiteDatabase db = helper.getWritableDatabase();
    db.insert(Tables.GroupAuthors.NAME, null/* nullColumnHack */, Tables.GroupAuthors.createValues(group, author));
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
      db.execSQL(Tables.CountryAuthors.createQuery());
      db.execSQL(Tables.Groups.CREATE_QUERY);
      db.execSQL(Tables.GroupAuthors.createQuery());
      db.execSQL(Tables.Authors.CREATE_QUERY);
      db.execSQL(Tables.Tracks.CREATE_QUERY);
      db.execSQL(Tables.AuthorTracks.createQuery());
      db.execSQL(Timestamps.CREATE_QUERY);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
      Log.d(TAG, "Upgrading database %d -> %d", oldVersion, newVersion);
      if (newVersion == 2) {
        //light update
        db.execSQL(Tables.Groups.CREATE_QUERY);
        db.execSQL(Tables.GroupAuthors.createQuery());
      } else {
        Utils.cleanupDb(db);
        onCreate(db);
      }
    }
  }
}
