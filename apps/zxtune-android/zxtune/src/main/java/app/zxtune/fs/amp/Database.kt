/**
 * @file
 * @brief DAL helper
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.amp

import android.content.Context
import android.database.Cursor
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper
import app.zxtune.Logger
import app.zxtune.TimeStamp
import app.zxtune.fs.dbhelpers.DBProvider
import app.zxtune.fs.dbhelpers.Grouping
import app.zxtune.fs.dbhelpers.Objects
import app.zxtune.fs.dbhelpers.Timestamps
import app.zxtune.fs.dbhelpers.Utils

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
 * Version 3
 *
 * CREATE TABLE author_pictures (author INTEGER NOT NULL, path TEXT NOT NULL, PRIMARY KEY(author, path))
 *
 */

private val LOG = Logger(Database::class.java.name)

private const val NAME = "amp.dascene.net"
private const val VERSION = 3

internal open class Database(context: Context) {
    private val helper = DBProvider(Helper(context))
    private val countryAuthors = Tables.CountryAuthors(helper)
    private val groupAuthors = Tables.GroupAuthors(helper)
    private val groups = Tables.Groups(helper)
    private val authors = Tables.Authors(helper)
    private val authorTracks = Tables.AuthorTracks(helper)
    private val tracks = Tables.Tracks(helper)
    private val timestamps = Timestamps(helper)
    private val findQuery = "SELECT * FROM authors LEFT OUTER JOIN tracks ON " + "tracks.${
        Tables.Tracks.getSelection(authorTracks.getIdsSelection("authors._id"))
    } " + "WHERE tracks.filename LIKE '%' || ? || '%'"

    fun close() = helper.close()

    open fun runInTransaction(cmd: Utils.ThrowingRunnable) = Utils.runInTransaction(helper, cmd)

    open fun getAuthorsLifetime(handleFilter: String, ttl: TimeStamp) =
        timestamps.getLifetime(Tables.Authors.NAME + handleFilter, ttl)

    open fun getCountryLifetime(country: Country, ttl: TimeStamp) =
        timestamps.getLifetime("countries" + country.id, ttl)

    open fun getAuthorTracksLifetime(author: Author, ttl: TimeStamp) =
        timestamps.getLifetime(Tables.Authors.NAME + author.id, ttl)

    open fun getGroupsLifetime(ttl: TimeStamp) = timestamps.getLifetime(Tables.Groups.NAME, ttl)

    open fun getGroupLifetime(group: Group, ttl: TimeStamp) =
        timestamps.getLifetime(Tables.Groups.NAME + group.id, ttl)

    open fun queryAuthors(handleFilter: String, visitor: Catalog.AuthorsVisitor) =
        Tables.Authors.getHandlesSelection(handleFilter).let {
            queryAuthorsInternal(it, visitor)
        }

    open fun queryAuthors(country: Country, visitor: Catalog.AuthorsVisitor) =
        Tables.Authors.getSelection(countryAuthors.getAuthorsIdsSelection(country)).let {
            queryAuthorsInternal(it, visitor)
        }

    open fun queryAuthors(group: Group, visitor: Catalog.AuthorsVisitor) =
        Tables.Authors.getSelection(groupAuthors.getAuthorsIdsSelection(group)).let {
            queryAuthorsInternal(it, visitor)
        }

    private fun queryAuthorsInternal(selection: String, visitor: Catalog.AuthorsVisitor) =
        helper.readableDatabase.query(Tables.Authors.NAME, null, selection, null, null, null, null)
            ?.use { cursor ->
                while (cursor.moveToNext()) {
                    visitor.accept(Tables.Authors.createAuthor(cursor))
                }
                cursor.count != 0
            } ?: false

    open fun queryTracks(author: Author, visitor: Catalog.TracksVisitor) =
        Tables.Tracks.getSelection(authorTracks.getTracksIdsSelection(author)).let {
            queryTracksInternal(it, visitor)
        }

    private fun queryTracksInternal(selection: String, visitor: Catalog.TracksVisitor) =
        helper.readableDatabase.query(Tables.Tracks.NAME, null, selection, null, null, null, null)
            ?.use { cursor ->
                while (cursor.moveToNext()) {
                    visitor.accept(Tables.Tracks.createTrack(cursor))
                }
                cursor.count != 0
            } ?: false

    open fun queryGroups(visitor: Catalog.GroupsVisitor) =
        helper.readableDatabase.query(Tables.Groups.NAME, null, null, null, null, null, null)
            ?.use { cursor ->
                while (cursor.moveToNext()) {
                    visitor.accept(Tables.Groups.createGroup(cursor))
                }
                cursor.count != 0
            } ?: false

    open fun findTracks(query: String, visitor: Catalog.FoundTracksVisitor) =
        helper.readableDatabase.rawQuery(findQuery, arrayOf(query))?.use { cursor ->
            while (cursor.moveToNext()) {
                val author = Tables.Authors.createAuthor(cursor)
                val track = Tables.Tracks.createTrack(cursor, Tables.Authors.FIELDS_COUNT)
                visitor.accept(author, track)
            }
        } ?: Unit

    open fun addCountryAuthor(country: Country, author: Author) =
        countryAuthors.add(country, author)

    open fun addGroup(group: Group) = groups.add(group)

    open fun addGroupAuthor(group: Group, author: Author) = groupAuthors.add(group, author)

    open fun addAuthor(obj: Author) = authors.add(obj)

    open fun addTrack(obj: Track) = tracks.add(obj)

    open fun addAuthorTrack(author: Author, track: Track) = authorTracks.add(author, track)
}

private class Helper(context: Context) : SQLiteOpenHelper(context, NAME, null, VERSION) {
    override fun onCreate(db: SQLiteDatabase) {
        LOG.d { "Creating database" }
        db.execSQL(Tables.CountryAuthors.CREATE_QUERY)
        db.execSQL(Tables.Groups.CREATE_QUERY)
        db.execSQL(Tables.GroupAuthors.CREATE_QUERY)
        db.execSQL(Tables.Authors.CREATE_QUERY)
        db.execSQL(Tables.Tracks.CREATE_QUERY)
        db.execSQL(Tables.AuthorTracks.CREATE_QUERY)
        db.execSQL(Tables.AuthorPictures.CREATE_QUERY)
        db.execSQL(Timestamps.CREATE_QUERY)
    }

    override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        LOG.d { "Upgrading database $oldVersion -> $newVersion" }
        if (newVersion == 2) {
            //light update
            db.execSQL(Tables.Groups.CREATE_QUERY)
            db.execSQL(Tables.GroupAuthors.CREATE_QUERY)
        } else {
            Utils.cleanupDb(db)
            onCreate(db)
        }
    }
}

private class Tables {
    class Authors(helper: DBProvider) : Objects(helper, NAME, FIELDS_COUNT) {
        fun add(obj: Author) = add(obj.id.toLong(), obj.handle, obj.realName)

        companion object {
            const val NAME = "authors"
            const val CREATE_QUERY =
                "CREATE TABLE authors (" + "_id INTEGER PRIMARY KEY, " + "handle TEXT NOT NULL, " + "real_name TEXT NOT NULL" + ");"
            const val FIELDS_COUNT = 3

            fun createAuthor(cursor: Cursor) = run {
                val id = cursor.getInt(0)
                val handle = cursor.getString(1)
                val realName = cursor.getString(2)
                Author(id, handle, realName)
            }

            fun getSelection(subquery: String) = "_id IN ($subquery)"

            fun getHandlesSelection(filter: String) =
                if (Catalog.NON_LETTER_FILTER == filter) "SUBSTR(handle, 1, 1) NOT BETWEEN 'A' AND 'Z' COLLATE NOCASE"
                else "handle LIKE '${filter}%'"

        }
    }

    class Tracks(helper: DBProvider) : Objects(helper, NAME, FIELDS_COUNT) {
        fun add(obj: Track) = add(obj.id.toLong(), obj.filename, obj.size.toLong())

        companion object {
            const val NAME = "tracks"
            const val CREATE_QUERY =
                "CREATE TABLE tracks (" + "_id INTEGER PRIMARY KEY, " + "filename TEXT NOT NULL, " + "size INTEGER NOT NULL" + ");"
            const val FIELDS_COUNT = 3

            fun createTrack(cursor: Cursor, fieldOffset: Int = 0) = run {
                val id = cursor.getInt(fieldOffset + 0)
                val filename = cursor.getString(fieldOffset + 1)
                val size = cursor.getInt(fieldOffset + 2)
                Track(id, filename, size)
            }

            fun getSelection(subquery: String) = "_id IN ($subquery)"
        }
    }

    class AuthorTracks(helper: DBProvider) : Grouping(helper, NAME, 32) {
        fun add(author: Author, track: Track) = add(author.id.toLong(), track.id.toLong())

        fun getTracksIdsSelection(author: Author): String = getIdsSelection(author.id.toLong())

        companion object {
            const val NAME = "author_tracks"
            val CREATE_QUERY: String = createQuery(NAME)
        }
    }

    @Deprecated("Not supported anymore")
    class AuthorPictures(helper: DBProvider) : Objects(helper, NAME, FIELDS_COUNT) {
        companion object {
            const val NAME = "author_pictures"
            const val CREATE_QUERY =
                "CREATE TABLE author_pictures (" + "author INTEGER NOT NULL, " + "path TEXT NOT NULL, " + "PRIMARY KEY(author, path)" + ");"
            const val FIELDS_COUNT = 2
        }
    }

    class CountryAuthors(helper: DBProvider) : Grouping(helper, NAME, 32) {
        fun add(country: Country, author: Author) = add(country.id.toLong(), author.id.toLong())

        fun getAuthorsIdsSelection(country: Country): String = getIdsSelection(country.id.toLong())

        companion object {
            const val NAME = "country_authors"
            val CREATE_QUERY: String = createQuery(NAME)
        }
    }

    class Groups(helper: DBProvider) : Objects(helper, NAME, FIELDS_COUNT) {
        fun add(obj: Group) = add(obj.id.toLong(), obj.name)

        companion object {
            const val NAME = "groups"
            const val CREATE_QUERY =
                "CREATE TABLE groups (" + "_id INTEGER PRIMARY KEY, " + "name TEXT NOT NULL" + ");"
            const val FIELDS_COUNT = 2

            fun createGroup(cursor: Cursor) = run {
                val id = cursor.getInt(0)
                val name = cursor.getString(1)
                Group(id, name)
            }
        }
    }

    class GroupAuthors(helper: DBProvider) : Grouping(helper, NAME, 32) {
        fun add(group: Group, author: Author) = add(group.id.toLong(), author.id.toLong())

        fun getAuthorsIdsSelection(group: Group): String = getIdsSelection(group.id.toLong())

        companion object {
            const val NAME = "group_authors"
            val CREATE_QUERY: String = createQuery(NAME)
        }
    }
}
