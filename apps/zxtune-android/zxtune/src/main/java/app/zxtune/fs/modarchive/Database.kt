/**
 * @file
 * @brief DAL helper
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.modarchive

import android.content.Context
import android.database.Cursor
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper
import app.zxtune.Logger
import app.zxtune.TimeStamp
import app.zxtune.fs.dbhelpers.*
import app.zxtune.fs.modarchive.Catalog.GenresVisitor

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

private val LOG = Logger(Database::class.java.name)
private const val NAME = "modarchive.org"
private const val VERSION = 1

internal open class Database(context: Context) {

    private val helper = DBProvider(Helper(context))
    private val authors = Tables.Authors(helper)
    private val authorTracks = Tables.AuthorTracks(helper)
    private val genres = Tables.Genres(helper)
    private val genreTracks = Tables.GenreTracks(helper)
    private val tracks = Tables.Tracks(helper)
    private val timestamps = Timestamps(helper)
    private val findQuery = "SELECT * FROM authors LEFT OUTER JOIN tracks ON " +
            "tracks.${Tables.Tracks.getSelection(authorTracks.getIdsSelection("authors._id"))} " +
            "WHERE tracks.filename || tracks.title LIKE '%' || ? || '%'"

    fun close() {
        helper.close()
    }

    open fun runInTransaction(cmd: Utils.ThrowingRunnable) = Utils.runInTransaction(helper, cmd)

    open fun getAuthorsLifetime(ttl: TimeStamp) = timestamps.getLifetime(Tables.Authors.NAME, ttl)

    open fun getGenresLifetime(ttl: TimeStamp) = timestamps.getLifetime(Tables.Genres.NAME, ttl)

    open fun getAuthorTracksLifetime(author: Author, ttl: TimeStamp) =
        timestamps.getLifetime(Tables.Authors.NAME + author.id, ttl)

    open fun getGenreTracksLifetime(genre: Genre, ttl: TimeStamp) =
        timestamps.getLifetime(Tables.Genres.NAME + genre.id, ttl)

    open fun queryAuthors(visitor: Catalog.AuthorsVisitor): Boolean {
        helper.readableDatabase.query(Tables.Authors.NAME, null, null, null, null, null, null)
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

    open fun queryGenres(visitor: GenresVisitor): Boolean {
        helper.readableDatabase.query(Tables.Genres.NAME, null, null, null, null, null, null)
            ?.use { cursor ->
                val count = cursor.count
                if (count != 0) {
                    visitor.setCountHint(count)
                    while (cursor.moveToNext()) {
                        visitor.accept(Tables.Genres.createGenre(cursor))
                    }
                    return true
                }
            }
        return false
    }

    open fun addGenre(obj: Genre) = genres.add(obj)

    open fun queryTracks(author: Author, visitor: Catalog.TracksVisitor): Boolean {
        val selection = Tables.Tracks.getSelection(authorTracks.getTracksIdsSelection(author))
        return queryTracksInternal(selection, visitor)
    }

    open fun queryTracks(genre: Genre, visitor: Catalog.TracksVisitor): Boolean {
        val selection = Tables.Tracks.getSelection(genreTracks.getTracksIdsSelection(genre))
        return queryTracksInternal(selection, visitor)
    }

    open fun queryRandomTracks(visitor: Catalog.TracksVisitor) {
        queryTracksInternal(null, "RANDOM() LIMIT 100", visitor)
    }

    private fun queryTracksInternal(selection: String?, visitor: Catalog.TracksVisitor): Boolean {
        return queryTracksInternal(selection, null, visitor)
    }

    private fun queryTracksInternal(
        selection: String?,
        order: String?,
        visitor: Catalog.TracksVisitor
    ): Boolean {
        helper.readableDatabase.query(Tables.Tracks.NAME, null, selection, null, null, null, order)
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
                    val track =
                        Tables.Tracks.createTrack(cursor, Tables.Authors.FIELDS_COUNT)
                    visitor.accept(author, track)
                }
            }
        }
    }

    open fun addTrack(obj: Track) = tracks.add(obj)

    open fun addAuthorTrack(author: Author, track: Track) = authorTracks.add(author, track)

    open fun addGenreTrack(genre: Genre, track: Track) = genreTracks.add(genre, track)

}

private class Helper(context: Context) :
    SQLiteOpenHelper(context, NAME, null, VERSION) {
    override fun onCreate(db: SQLiteDatabase) {
        LOG.d { "Creating database" }
        db.execSQL(Tables.Authors.CREATE_QUERY)
        db.execSQL(Tables.AuthorTracks.CREATE_QUERY)
        db.execSQL(Tables.Genres.CREATE_QUERY)
        db.execSQL(Tables.GenreTracks.CREATE_QUERY)
        db.execSQL(Tables.Tracks.CREATE_QUERY)
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

        fun add(obj: Author) = add(obj.id.toLong(), obj.alias)

        companion object {
            const val NAME = "authors"
            const val CREATE_QUERY = "CREATE TABLE authors (" +
                    "_id INTEGER PRIMARY KEY, " +
                    "alias TEXT NOT NULL" +
                    ");"
            const val FIELDS_COUNT = 2

            fun createAuthor(cursor: Cursor) = run {
                val id = cursor.getInt(0)
                val alias = cursor.getString(1)
                Author(id, alias)
            }
        }
    }

    class Tracks(helper: DBProvider) : Objects(helper, NAME, FIELDS_COUNT) {
        fun add(obj: Track) = add(obj.id, obj.filename, obj.title, obj.size)

        companion object {
            const val NAME = "tracks"
            const val CREATE_QUERY = "CREATE TABLE tracks (" +
                    "_id INTEGER PRIMARY KEY, " +
                    "filename TEXT NOT NULL, " +
                    "title TEXT NOT NULL, " +
                    "size INTEGER NOT NULL" +
                    ");"
            const val FIELDS_COUNT = 4

            fun createTrack(cursor: Cursor, fieldOffset: Int = 0) = run {
                val id = cursor.getInt(fieldOffset + 0)
                val filename = cursor.getString(fieldOffset + 1)
                val title = cursor.getString(fieldOffset + 2)
                val size = cursor.getInt(fieldOffset + 3)
                Track(id, filename, title, size)
            }

            fun getSelection(subquery: String) = "_id IN ($subquery)"
        }
    }

    class AuthorTracks(helper: DBProvider) : Grouping(helper, NAME, 32) {
        fun add(author: Author, track: Track) = add(author.id.toLong(), track.id.toLong())

        fun getTracksIdsSelection(author: Author): String =
            getIdsSelection(author.id.toLong())

        companion object {
            const val NAME = "author_tracks"
            val CREATE_QUERY: String = createQuery(NAME)
        }
    }

    class Genres(helper: DBProvider) : Objects(helper, NAME, FIELDS_COUNT) {

        fun add(obj: Genre) = add(obj.id.toLong(), obj.name, obj.tracks.toLong())

        companion object {
            const val NAME = "genres"
            const val CREATE_QUERY = "CREATE TABLE genres (" +
                    "_id INTEGER PRIMARY KEY, " +
                    "name TEXT NOT NULL, " +
                    "tracks INTEGER NOT NULL" +
                    ");"
            const val FIELDS_COUNT = 3

            fun createGenre(cursor: Cursor) = run {
                val id = cursor.getInt(0)
                val name = cursor.getString(1)
                val tracks = cursor.getInt(2)
                Genre(id, name, tracks)
            }
        }
    }

    class GenreTracks(helper: DBProvider) : Grouping(helper, NAME, 32) {
        fun add(genre: Genre, track: Track) = add(genre.id.toLong(), track.id.toLong())

        fun getTracksIdsSelection(genre: Genre): String = getIdsSelection(genre.id.toLong())

        companion object {
            const val NAME = "genre_tracks"
            val CREATE_QUERY: String = createQuery(NAME)
        }
    }
}
