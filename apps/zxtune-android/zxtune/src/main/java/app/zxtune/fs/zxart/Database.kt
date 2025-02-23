/**
 * @file
 * @brief DAL helper
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.zxart

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

private val LOG = Logger(Database::class.java.name)
private const val NAME = "www.zxart.ee"
private const val VERSION = 3

internal open class Database(context: Context) {

    private val helper = DBProvider(Helper(context))
    private val authors = Tables.Authors(helper)
    private val authorsTracks = Tables.AuthorsTracks(helper)
    private val parties = Tables.Parties(helper)
    private val partiesTracks = Tables.PartiesTracks(helper)
    private val tracks = Tables.Tracks(helper)
    private val timestamps = Timestamps(helper)
    private val findQuery = "SELECT * FROM authors LEFT OUTER JOIN tracks ON " + "tracks.${
        Tables.Tracks.getSelection(authorsTracks.getIdsSelection("authors._id"))
    } " + "WHERE tracks.filename || tracks.title LIKE '%' || ? || '%'"

    fun close() {
        helper.close()
    }

    open fun runInTransaction(cmd: Utils.ThrowingRunnable) = Utils.runInTransaction(helper, cmd)

    open fun getAuthorsLifetime(ttl: TimeStamp) = timestamps.getLifetime(Tables.Authors.NAME, ttl)

    open fun getAuthorTracksLifetime(author: Author, ttl: TimeStamp) =
        timestamps.getLifetime(Tables.Authors.NAME + author.id, ttl)

    open fun getPartiesLifetime(ttl: TimeStamp) = timestamps.getLifetime(Tables.Parties.NAME, ttl)

    open fun getPartyTracksLifetime(party: Party, ttl: TimeStamp) =
        timestamps.getLifetime(Tables.Parties.NAME + party.id, ttl)

    open fun getTopLifetime(ttl: TimeStamp) = timestamps.getLifetime(Tables.Tracks.NAME, ttl)

    open fun queryAuthors(visitor: Catalog.Visitor<Author>) =
        helper.readableDatabase.query(Tables.Authors.NAME, null, null, null, null, null, null)
            ?.use { cursor ->
                while (cursor.moveToNext()) {
                    visitor.accept(Tables.Authors.createAuthor(cursor))
                }
                cursor.count != 0
            } ?: false

    open fun addAuthor(obj: Author) = authors.add(obj)

    open fun queryParties(visitor: Catalog.Visitor<Party>) =
        helper.readableDatabase.query(Tables.Parties.NAME, null, null, null, null, null, null)
            ?.use { cursor ->
                while (cursor.moveToNext()) {
                    visitor.accept(Tables.Parties.createParty(cursor))
                }
                cursor.count != 0
            } ?: false

    open fun addParty(obj: Party) = parties.add(obj)

    open fun queryAuthorTracks(author: Author, visitor: Catalog.Visitor<Track>) =
        Tables.Tracks.getSelection(authorsTracks.getTracksIdsSelection(author)).let {
            queryTracks(it, visitor)
        }

    open fun queryPartyTracks(party: Party, visitor: Catalog.Visitor<Track>) =
        Tables.Tracks.getSelection(partiesTracks.getTracksIdsSelection(party)).let {
            queryTracks(it, visitor)
        }

    private fun queryTracks(selection: String, visitor: Catalog.Visitor<Track>) =
        helper.readableDatabase.query(Tables.Tracks.NAME, null, selection, null, null, null, null)
            .let { cursor ->
                queryTracks(cursor, visitor)
            }

    open fun queryTopTracks(limit: Int, visitor: Catalog.Visitor<Track>) =
        helper.readableDatabase.query(
            Tables.Tracks.NAME, null, null, null, null, null, "votes DESC", limit.toString()
        ).let { cursor ->
            queryTracks(cursor, visitor)
        }

    open fun findTracks(query: String, visitor: Catalog.FoundTracksVisitor) {
        helper.readableDatabase.rawQuery(findQuery, arrayOf(query))?.use { cursor ->
            while (cursor.moveToNext()) {
                val author = Tables.Authors.createAuthor(cursor)
                val track = Tables.Tracks.createTrack(cursor, Tables.Authors.FIELDS_COUNT)
                visitor.accept(author, track)
            }
        }
    }

    open fun addTrack(track: Track) = tracks.add(track)

    open fun addAuthorTrack(author: Author, track: Track) = authorsTracks.add(author, track)

    open fun addPartyTrack(party: Party, track: Track) = partiesTracks.add(party, track)
}

private fun queryTracks(cursor: Cursor?, visitor: Catalog.Visitor<Track>): Boolean {
    cursor?.use {
        while (cursor.moveToNext()) {
            visitor.accept(Tables.Tracks.createTrack(cursor))
        }
        return cursor.count != 0
    }
    return false
}

private class Helper constructor(context: Context) :
    SQLiteOpenHelper(context, NAME, null, VERSION) {
    override fun onCreate(db: SQLiteDatabase) {
        LOG.d { "Creating database" }
        db.execSQL(Tables.Authors.CREATE_QUERY)
        db.execSQL(Tables.Parties.CREATE_QUERY)
        db.execSQL(Tables.Tracks.CREATE_QUERY)
        db.execSQL(Tables.AuthorsTracks.CREATE_QUERY)
        db.execSQL(Tables.PartiesTracks.CREATE_QUERY)
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
        fun add(obj: Author) = add(obj.id.toLong(), obj.nickname, obj.name)

        companion object {
            const val NAME = "authors"
            const val CREATE_QUERY =
                "CREATE TABLE authors (" + "_id  INTEGER PRIMARY KEY, " + "nickname TEXT NOT NULL, " + "name  TEXT);"
            const val FIELDS_COUNT = 3

            fun createAuthor(cursor: Cursor) = run {
                val id = cursor.getInt(0)
                val name = cursor.getString(2)
                val nickname = cursor.getString(1)
                Author(id, nickname, name)
            }
        }
    }

    class Parties(helper: DBProvider) : Objects(helper, NAME, FIELDS_COUNT) {
        fun add(obj: Party) = add(obj.id.toLong(), obj.name, obj.year.toLong())

        companion object {
            const val NAME = "parties"
            const val CREATE_QUERY =
                "CREATE TABLE parties (" + "_id INTEGER PRIMARY KEY, " + "name TEXT NOT NULL, " + "year INTEGER NOT NULL);"
            const val FIELDS_COUNT = 3

            fun createParty(cursor: Cursor) = run {
                val id = cursor.getInt(0)
                val name = cursor.getString(1)
                val year = cursor.getInt(2)
                Party(id, name, year)
            }
        }
    }

    class Tracks(helper: DBProvider) : Objects(helper, NAME, FIELDS_COUNT) {
        fun add(obj: Track) = with(obj) {
            add(id, filename, title, votes, duration, year, compo, partyplace)
        }

        companion object {
            const val NAME = "tracks"
            const val CREATE_QUERY =
                "CREATE TABLE tracks (" + "_id INTEGER PRIMARY KEY, " + "filename TEXT NOT NULL, " + "title TEXT, " + "votes TEXT, " + "duration INTEGER, " + "year INTEGER, " + "compo TEXT, " + "partyplace INTEGER);"
            const val FIELDS_COUNT = 8

            fun createTrack(cursor: Cursor, fieldOffset: Int = 0) = run {
                val id = cursor.getInt(fieldOffset + 0)
                val filename = cursor.getString(fieldOffset + 1)
                val title = cursor.getString(fieldOffset + 2)
                val votes = cursor.getString(fieldOffset + 3)
                val duration = cursor.getString(fieldOffset + 4)
                val year = cursor.getInt(fieldOffset + 5)
                val compo = cursor.getString(fieldOffset + 6)
                val partyplace = cursor.getInt(fieldOffset + 7)
                Track(id, filename, title, votes, duration, year, compo, partyplace)
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

    class PartiesTracks(helper: DBProvider) : Grouping(helper, NAME, 32) {
        fun add(party: Party, track: Track) = add(party.id.toLong(), track.id.toLong())

        fun getTracksIdsSelection(party: Party): String = getIdsSelection(party.id.toLong())

        companion object {
            const val NAME = "parties_tracks"
            val CREATE_QUERY: String = createQuery(NAME)
        }
    }
}
