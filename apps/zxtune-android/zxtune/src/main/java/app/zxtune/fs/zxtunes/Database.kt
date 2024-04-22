/**
 * @file
 * @brief DAL helper
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.zxtunes

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
 *
 * Version 3
 * Add Authors.has_photo field
 */

private val LOG = Logger(Database::class.java.name)
private const val NAME = "www.zxtunes.com"
private const val VERSION = 3

internal open class Database(context: Context) {

    private val helper = DBProvider(Helper(context))
    private val authors = Tables.Authors(helper)
    private val authorsTracks = Tables.AuthorsTracks(helper)
    private val tracks = Tables.Tracks(helper)
    private val timestamps = Timestamps(helper)
    private val findQuery = "SELECT * " +
            "FROM authors LEFT OUTER JOIN tracks ON " +
            "tracks.${Tables.Tracks.getSelection(authorsTracks.getIdsSelection("authors._id"))}" +
            " WHERE tracks.filename || tracks.title LIKE '%' || ? || '%'"

    fun close() {
        helper.close()
    }

    open fun runInTransaction(cmd: Utils.ThrowingRunnable) = Utils.runInTransaction(helper, cmd)

    open fun getAuthorsLifetime(ttl: TimeStamp) = timestamps.getLifetime(Tables.Authors.NAME, ttl)

    open fun getAuthorTracksLifetime(author: Author, ttl: TimeStamp) =
        timestamps.getLifetime(Tables.Authors.NAME + author.id, ttl)

    open fun queryAuthors(visitor: Catalog.AuthorsVisitor, id: Int? = null): Boolean {
        helper.readableDatabase.query(
            Tables.Authors.NAME, null, id?.let { "_id = ?" },
            id?.let { arrayOf(it.toString()) }, null,
            null, null
        )
            ?.use { cursor ->
                val count = cursor.count
                if (count != 0) {
                    visitor.setCountHint(count)
                    while (cursor.moveToNext()) {
                        visitor.accept(Tables.Authors.createAuthor(cursor))
                    }
                    return true
                }
            }
        return false
    }

    open fun addAuthor(obj: Author) = authors.add(obj)

    open fun queryAuthorTracks(author: Author, visitor: Catalog.TracksVisitor): Boolean {
        val selection = Tables.Tracks.getSelection(authorsTracks.getTracksIdsSelection(author))
        return queryTracks(selection, visitor)
    }

    private fun queryTracks(selection: String, visitor: Catalog.TracksVisitor): Boolean {
        helper.readableDatabase.query(Tables.Tracks.NAME, null, selection, null, null, null, null)
            ?.use { cursor ->
                val count = cursor.count
                if (count != 0) {
                    visitor.setCountHint(count)
                    while (cursor.moveToNext()) {
                        visitor.accept(Tables.Tracks.createTrack(cursor))
                    }
                    return true
                }
            }
        return false
    }

    open fun findTracks(query: String, visitor: Catalog.FoundTracksVisitor) {
        helper.readableDatabase.rawQuery(findQuery, arrayOf(query))?.use { cursor ->
            val count = cursor.count
            if (count != 0) {
                visitor.setCountHint(count)
                while (cursor.moveToNext()) {
                    val author = Tables.Authors.createAuthor(cursor)
                    val track = Tables.Tracks.createTrack(cursor, Tables.Authors.FIELDS_COUNT)
                    visitor.accept(author, track)
                }
            }
        }
    }

    open fun addTrack(obj: Track) = tracks.add(obj)

    open fun addAuthorTrack(author: Author, track: Track) = authorsTracks.add(author, track)
}

private class Helper constructor(context: Context) :
    SQLiteOpenHelper(context, NAME, null, VERSION) {
    override fun onCreate(db: SQLiteDatabase) {
        LOG.d { "Creating database" }
        db.execSQL(Tables.Authors.CREATE_QUERY)
        db.execSQL(Tables.Tracks.CREATE_QUERY)
        db.execSQL(Tables.AuthorsTracks.CREATE_QUERY)
        db.execSQL(Timestamps.CREATE_QUERY)
    }

    override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        LOG.d { "Upgrading database $oldVersion -> $newVersion" }
        Utils.cleanupDb(db)
        onCreate(db)
    }
}

private class Tables {
    class Authors(helper: DBProvider) : Objects(helper, NAME, FIELDS_COUNT) {
        fun add(obj: Author) =
            add(obj.id.toLong(), obj.nickname, obj.name, if (requireNotNull(obj.hasPhoto)) 1 else 0)

        companion object {
            const val NAME = "authors"
            const val CREATE_QUERY = "CREATE TABLE authors (" +
                    "_id  INTEGER PRIMARY KEY, " +
                    "nickname TEXT NOT NULL, " +
                    "name TEXT, " +
                    "has_photo INTEGER);"
            const val FIELDS_COUNT = 4

            fun createAuthor(cursor: Cursor) = run {
                val id = cursor.getInt(0)
                val nickname = cursor.getString(1)
                val name = cursor.getString(2)
                val hasPhoto = cursor.getInt(3) != 0
                Author(id, nickname, name, hasPhoto)
            }
        }
    }

    class Tracks(helper: DBProvider) : Objects(helper, NAME, FIELDS_COUNT) {
        fun add(obj: Track) = add(obj.id, obj.filename, obj.title, obj.duration, obj.date)

        companion object {
            const val NAME = "tracks"
            const val CREATE_QUERY = "CREATE TABLE tracks (" +
                    "_id INTEGER PRIMARY KEY, " +
                    "filename TEXT NOT NULL, " +
                    "title TEXT, " +
                    "duration INTEGER, " +
                    "date INTEGER);"
            const val FIELDS_COUNT = 5

            fun createTrack(cursor: Cursor, fieldOffset: Int = 0) = run {
                val id = cursor.getInt(fieldOffset + 0)
                val filename = cursor.getString(fieldOffset + 1)
                val title = cursor.getString(fieldOffset + 2)
                val duration = cursor.getInt(fieldOffset + 3)
                val date = cursor.getInt(fieldOffset + 4)
                Track(id, filename, title, duration, date)
            }

            fun getSelection(subquery: String) = "_id IN ($subquery)"
        }
    }

    class AuthorsTracks(helper: DBProvider) : Grouping(helper, NAME, 32) {
        fun add(author: Author, track: Track) = add(author.id.toLong(), track.id.toLong())

        fun getTracksIdsSelection(author: Author): String = getIdsSelection(author.id.toLong())

        companion object {
            const val NAME = "authors_tracks"
            val CREATE_QUERY: String = createQuery(NAME)
        }
    }
}
