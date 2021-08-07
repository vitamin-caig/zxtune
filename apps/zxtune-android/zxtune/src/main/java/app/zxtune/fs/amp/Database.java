/**
 * @file
 * @brief DAL helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.amp;

import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;

import java.io.IOException;

import app.zxtune.Log;
import app.zxtune.TimeStamp;
import app.zxtune.fs.amp.Catalog.AuthorsVisitor;
import app.zxtune.fs.dbhelpers.DBProvider;
import app.zxtune.fs.dbhelpers.Grouping;
import app.zxtune.fs.dbhelpers.Objects;
import app.zxtune.fs.dbhelpers.Timestamps;
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
 * group_authors with 32-bit grouping
 *
 */

final class Database {

  private static final String TAG = Database.class.getName();

  private static final String NAME = "amp.dascene.net";
  private static final int VERSION = 2;

  static final class Tables {

    static final class Authors extends Objects {

      enum Fields {
        _id, handle, real_name
      }

      static final String NAME = "authors";

      static final String CREATE_QUERY = "CREATE TABLE authors (" +
              "_id INTEGER PRIMARY KEY, " +
              "handle TEXT NOT NULL, " +
              "real_name TEXT NOT NULL" +
              ");";

      Authors(DBProvider helper) {
        super(helper, NAME, Fields.values().length);
      }

      final void add(Author obj) {
        add(obj.getId(), obj.getHandle(), obj.getRealName());
      }

      static Author createAuthor(Cursor cursor) {
        final int id = cursor.getInt(Fields._id.ordinal());
        final String handle = cursor.getString(Fields.handle.ordinal());
        final String realName = cursor.getString(Fields.real_name.ordinal());
        return new Author(id, handle, realName);
      }

      static String getSelection(String subquery) {
        return Fields._id + " IN (" + subquery + ")";
      }
    }

    static final class Tracks extends Objects {

      enum Fields {
        _id, filename, size
      }

      static final String NAME = "tracks";

      static final String CREATE_QUERY = "CREATE TABLE tracks (" +
              "_id INTEGER PRIMARY KEY, " +
              "filename TEXT NOT NULL, " +
              "size INTEGER NOT NULL" +
              ");";

      Tracks(DBProvider helper) {
        super(helper, NAME, Fields.values().length);
      }

      final void add(Track obj) {
        add(obj.id, obj.filename, obj.size);
      }

      static Track createTrack(Cursor cursor) {
        return createTrack(cursor, 0);
      }

      static Track createTrack(Cursor cursor, int fieldOffset) {
        final int id = cursor.getInt(fieldOffset + Fields._id.ordinal());
        final String filename = cursor.getString(fieldOffset + Fields.filename.ordinal());
        final int size = cursor.getInt(fieldOffset + Fields.size.ordinal());
        return new Track(id, filename, size);
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
        add(author.getId(), track.id);
      }

      final String getTracksIdsSelection(Author author) {
        return getIdsSelection(author.getId());
      }
    }

    static final class CountryAuthors extends Grouping {

      static final String NAME = "country_authors";
      static final String CREATE_QUERY = Grouping.createQuery(NAME);

      CountryAuthors(DBProvider helper) {
        super(helper, NAME, 32);
      }

      final void add(Country country, Author author) {
        add(country.id, author.getId());
      }

      final String getAuthorsIdsSelection(Country country) {
        return getIdsSelection(country.id);
      }
    }

    static final class Groups extends Objects {

      enum Fields {
        _id, name
      }

      static final String NAME = "groups";

      static final String CREATE_QUERY = "CREATE TABLE groups (" +
              "_id INTEGER PRIMARY KEY, " +
              "name TEXT NOT NULL" +
              ");";

      Groups(DBProvider helper) {
        super(helper, NAME, Fields.values().length);
      }

      final void add(Group obj) {
        add(obj.id, obj.name);
      }

      static Group createGroup(Cursor cursor) {
        final int id = cursor.getInt(Fields._id.ordinal());
        final String name = cursor.getString(Fields.name.ordinal());
        return new Group(id, name);
      }
    }

    static final class GroupAuthors extends Grouping {

      static final String NAME = "group_authors";
      static final String CREATE_QUERY = Grouping.createQuery(NAME);

      GroupAuthors(DBProvider helper) {
        super(helper, NAME, 32);
      }

      final void add(Group group, Author author) {
        add(group.id, author.getId());
      }

      final String getAuthorsIdsSelection(Group group) {
        return getIdsSelection(group.id);
      }
    }
  }

  private final DBProvider helper;
  private final Tables.CountryAuthors countryAuthors;
  private final Tables.GroupAuthors groupAuthors;
  private final Tables.Groups groups;
  private final Tables.Authors authors;
  private final Tables.AuthorTracks authorTracks;
  private final Tables.Tracks tracks;
  private final Timestamps timestamps;
  private final String findQuery;

  Database(Context context) {
    this.helper = new DBProvider(Helper.create(context));
    this.countryAuthors = new Tables.CountryAuthors(helper);
    this.groupAuthors = new Tables.GroupAuthors(helper);
    this.groups = new Tables.Groups(helper);
    this.authors = new Tables.Authors(helper);
    this.authorTracks = new Tables.AuthorTracks(helper);
    this.tracks = new Tables.Tracks(helper);
    this.timestamps = new Timestamps(helper);
    this.findQuery = "SELECT * " +
            "FROM authors LEFT OUTER JOIN tracks ON " +
            "tracks." + Tables.Tracks.getSelection(authorTracks.getIdsSelection("authors._id")) +
            " WHERE tracks.filename LIKE '%' || ? || '%'";
  }

  final void runInTransaction(Utils.ThrowingRunnable cmd) throws IOException {
    Utils.runInTransaction(helper, cmd);
  }

  final Timestamps.Lifetime getAuthorsLifetime(String handleFilter, TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Authors.NAME + handleFilter, ttl);
  }

  final Timestamps.Lifetime getCountryLifetime(Country country, TimeStamp ttl) {
    return timestamps.getLifetime("countries" + country.id, ttl);
  }

  final Timestamps.Lifetime getAuthorTracksLifetime(Author author, TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Authors.NAME + author.getId(), ttl);
  }

  final Timestamps.Lifetime getGroupsLifetime(TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Groups.NAME, ttl);
  }

  final Timestamps.Lifetime getGroupLifetime(Group group, TimeStamp ttl) {
    return timestamps.getLifetime(Tables.Groups.NAME + group.id, ttl);
  }

  final boolean queryAuthors(String handleFilter, AuthorsVisitor visitor) {
    final String selection = Catalog.NON_LETTER_FILTER.equals(handleFilter)
            ? "SUBSTR(" + Tables.Authors.Fields.handle + ", 1, 1) NOT BETWEEN 'A' AND 'Z' COLLATE NOCASE"
            : Tables.Authors.Fields.handle + " LIKE '" + handleFilter + "%'";
    return queryAuthorsInternal(selection, visitor);
  }

  final boolean queryAuthors(Country country, AuthorsVisitor visitor) {
    final String selection = Tables.Authors.getSelection(countryAuthors.getAuthorsIdsSelection(country));
    return queryAuthorsInternal(selection, visitor);
  }

  final boolean queryAuthors(Group group, AuthorsVisitor visitor) {
    final String selection = Tables.Authors.getSelection(groupAuthors.getAuthorsIdsSelection(group));
    return queryAuthorsInternal(selection, visitor);
  }

  private boolean queryAuthorsInternal(String selection, AuthorsVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
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

  final boolean queryTracks(Author author, Catalog.TracksVisitor visitor) {
    final String selection = Tables.Tracks.getSelection(authorTracks.getTracksIdsSelection(author));
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

  final boolean queryGroups(Catalog.GroupsVisitor visitor) {
    final SQLiteDatabase db = helper.getReadableDatabase();
    final Cursor cursor = db.query(Tables.Groups.NAME, null, null, null, null, null, null);
    try {
      final int count = cursor.getCount();
      if (count != 0) {
        visitor.setCountHint(count);
        while (cursor.moveToNext()) {
          visitor.accept(Tables.Groups.createGroup(cursor));
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

  final void addCountryAuthor(Country country, Author author) {
    countryAuthors.add(country, author);
  }

  final void addGroup(Group group) {
    groups.add(group);
  }

  final void addGroupAuthor(Group group, Author author) {
    groupAuthors.add(group, author);
  }

  final void addAuthor(Author obj) {
    authors.add(obj);
  }

  final void addTrack(Track obj) {
    tracks.add(obj);
  }

  final void addAuthorTrack(Author author, Track track) {
    authorTracks.add(author, track);
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
      db.execSQL(Tables.CountryAuthors.CREATE_QUERY);
      db.execSQL(Tables.Groups.CREATE_QUERY);
      db.execSQL(Tables.GroupAuthors.CREATE_QUERY);
      db.execSQL(Tables.Authors.CREATE_QUERY);
      db.execSQL(Tables.Tracks.CREATE_QUERY);
      db.execSQL(Tables.AuthorTracks.CREATE_QUERY);
      db.execSQL(Timestamps.CREATE_QUERY);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
      Log.d(TAG, "Upgrading database %d -> %d", oldVersion, newVersion);
      if (newVersion == 2) {
        //light update
        db.execSQL(Tables.Groups.CREATE_QUERY);
        db.execSQL(Tables.GroupAuthors.CREATE_QUERY);
      } else {
        Utils.cleanupDb(db);
        onCreate(db);
      }
    }
  }
}
