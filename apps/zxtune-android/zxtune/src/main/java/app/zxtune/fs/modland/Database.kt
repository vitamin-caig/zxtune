/**
 * @file
 * @brief DAL helper
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.modland

import android.content.Context
import android.database.Cursor
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper
import android.net.Uri
import app.zxtune.Logger
import app.zxtune.TimeStamp
import app.zxtune.fs.dbhelpers.*

/**
 * Version 1
 *
 * CREATE TABLE {authors,collections,formats} (_id INTEGER PRIMARY KEY, name TEXT NOT NULL, tracks INTEGER)
 * CREATE TABLE tracks (_id INTEGER PRIMARY KEY, path TEXT NOT NULL, size INTEGER)
 * CREATE TABLE {authors,collections,formats}_tracks (hash INTEGER UNIQUE, group_id INTEGER, track_id INTEGER)
 *
 * use hash as 10000000000 * group_id + track_id to support multiple insertions of same pair
 *
 *
 * Version 2
 *
 * Use standard grouping for {authors,collections,formats}_tracks
 * Use timestamps
 */
private val LOG = Logger(Database::class.java.name)
private const val NAME = "modland.com"
private const val VERSION = 2

internal open class Database(context: Context) {

    private val helper = DBProvider(Helper(context))
    private val groups = HashMap<String, TablesInternal.Groups>()
    private val groupTracks = HashMap<String, TablesInternal.GroupTracks>()
    private val tracks = TablesInternal.Tracks(helper)
    private val timestamps = Timestamps(helper)

    init {
        for (group in Tables.LIST) {
            groups[group] = TablesInternal.Groups(helper, group)
            groupTracks[group] = TablesInternal.GroupTracks(helper, group)
        }
    }

    fun close() {
        helper.close()
    }

    open fun runInTransaction(cmd: Utils.ThrowingRunnable) = Utils.runInTransaction(helper, cmd)

    open fun getGroupsLifetime(category: String, filter: String, ttl: TimeStamp) =
        timestamps.getLifetime(category + filter, ttl)

    open fun getGroupTracksLifetime(category: String, id: Int, ttl: TimeStamp) =
        timestamps.getLifetime(category + id, ttl)

    open fun queryGroups(
        category: String,
        filter: String,
        visitor: Catalog.GroupsVisitor
    ): Boolean {
        val selection = TablesInternal.Groups.getFilterSelection(filter)
        helper.readableDatabase.query(category, null, selection, null, null, null, null)
            ?.use { cursor ->
                val count = cursor.count
                if (count != 0) {
                    visitor.setCountHint(count)
                    while (cursor.moveToNext()) {
                        visitor.accept(TablesInternal.Groups.createGroup(cursor))
                    }
                    return true
                }
            }
        return false
    }

    open fun queryGroup(category: String, id: Int): Group? {
        val selection = TablesInternal.Groups.getIdSelection(id)
        return helper.readableDatabase.query(
            category, null, selection, null, null, null, null
        )?.use { cursor ->
            if (cursor.moveToNext()) TablesInternal.Groups.createGroup(cursor) else null
        }
    }

    open fun addGroup(category: String, obj: Group) = groups[category]?.add(obj)

    open fun queryTracks(category: String, id: Int, visitor: Catalog.TracksVisitor): Boolean {
        val selection = TablesInternal.Tracks.getSelection(
            groupTracks[category]!!.getIdsSelection(id.toLong())
        )
        helper.readableDatabase.query(TablesInternal.Tracks.NAME, null, selection, null, null, null, null)
            ?.use { cursor ->
                val count = cursor.count
                if (count != 0) {
                    visitor.setCountHint(count)
                    while (cursor.moveToNext()) {
                        visitor.accept(TablesInternal.Tracks.createTrack(cursor))
                    }
                    return true
                }
            }
        return false
    }

    open fun findTrack(category: String, id: Int, filename: String): Track? {
        val encodedFilename =
            Uri.encode(filename).replace("!", "%21").replace("'", "%27").replace("(", "%28")
                .replace(")", "%29")
        val selection =
            TablesInternal.Tracks.getSelectionWithPath(groupTracks[category]!!.getIdsSelection(id.toLong()))
        val selectionArgs = arrayOf("%/$encodedFilename")
        helper.readableDatabase.query(
            TablesInternal.Tracks.NAME, null, selection, selectionArgs, null,
            null, null
        )?.use { cursor ->
            if (cursor.moveToFirst()) {
                return TablesInternal.Tracks.createTrack(cursor)
            }
        }
        return null
    }

    open fun addTrack(obj: Track) = tracks.add(obj)

    open fun addGroupTrack(category: String, id: Int, obj: Track) =
        groupTracks[category]?.add(id.toLong(), obj.id)

    object Tables {
        val LIST = arrayOf(Authors.NAME, Collections.NAME, Formats.NAME)

        object Authors {
            const val NAME = "authors"
        }

        object Collections {
            const val NAME = "collections"
        }

        object Formats {
            const val NAME = "formats"
        }
    }
}

private class Helper constructor(context: Context) :
    SQLiteOpenHelper(context, NAME, null, VERSION) {
    override fun onCreate(db: SQLiteDatabase) {
        LOG.d { "Creating database" }
        for (table in Database.Tables.LIST) {
            db.execSQL(TablesInternal.Groups.getCreateQuery(table))
            db.execSQL(TablesInternal.GroupTracks.getCreateQuery(table))
        }
        db.execSQL(TablesInternal.Tracks.CREATE_QUERY)
        db.execSQL(Timestamps.CREATE_QUERY)
    }

    override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        LOG.d { "Upgrading database $oldVersion -> $newVersion" }
        Utils.cleanupDb(db)
        onCreate(db)
    }
}

private object TablesInternal {
    class Groups(helper: DBProvider, name: String) : Objects(helper, name, FIELDS_COUNT) {
        fun add(obj: Group) = add(obj.id.toLong(), obj.name, obj.tracks.toLong())

        companion object {
            fun getCreateQuery(name: String) =
                "CREATE TABLE $name (_id INTEGER PRIMARY KEY, name TEXT NOT NULL, tracks INTEGER);"

            const val FIELDS_COUNT = 3

            fun createGroup(cursor: Cursor) = run {
                val id = cursor.getInt(0)
                val title = cursor.getString(1)
                val tracks = cursor.getInt(2)
                Group(id, title, tracks)
            }

            fun getFilterSelection(filter: String) = if (filter == "#")
                "SUBSTR(name, 1, 1) NOT BETWEEN 'A' AND 'Z' COLLATE NOCASE"
            else
                "name LIKE '$filter%'"

            fun getIdSelection(id: Int) = "_id = $id"
        }
    }

    class Tracks(helper: DBProvider) : Objects(helper, NAME, FIELDS_COUNT) {
        fun add(obj: Track) = add(obj.id, obj.path, obj.size.toLong())

        companion object {
            const val NAME = "tracks"
            const val CREATE_QUERY =
                "CREATE TABLE tracks (_id INTEGER PRIMARY KEY, path TEXT NOT NULL, size INTEGER);"
            const val FIELDS_COUNT = 3

            fun createTrack(cursor: Cursor) = run {
                val id = cursor.getInt(0)
                val path = cursor.getString(1)
                val size = cursor.getInt(2)
                Track(id.toLong(), path, size)
            }

            fun getSelection(subquery: String) = "_id IN ($subquery)"

            fun getSelectionWithPath(subquery: String) = getSelection(subquery) + " AND path LIKE ?"
        }
    }

    class GroupTracks(helper: DBProvider, category: String) : Grouping(helper, name(category), 32) {
        companion object {
            fun name(category: String) = "${category}_tracks"

            fun getCreateQuery(category: String): String = createQuery(name(category))
        }
    }
}
