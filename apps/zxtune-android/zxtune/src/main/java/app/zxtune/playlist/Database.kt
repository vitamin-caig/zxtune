package app.zxtune.playlist

import android.content.Context
import android.database.Cursor
import androidx.annotation.VisibleForTesting
import androidx.core.util.Consumer
import androidx.room.Dao
import androidx.room.Embedded
import androidx.room.Entity
import androidx.room.Index
import androidx.room.Insert
import androidx.room.PrimaryKey
import androidx.room.Query
import androidx.room.RawQuery
import androidx.room.RewriteQueriesToDropUnusedColumns
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.room.Transaction
import androidx.room.TypeConverter
import androidx.room.TypeConverters
import androidx.room.migration.Migration
import androidx.sqlite.db.SimpleSQLiteQuery
import androidx.sqlite.db.SupportSQLiteDatabase
import androidx.sqlite.db.SupportSQLiteQuery
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import app.zxtune.fs.dbhelpers.DBStatistics
import app.zxtune.playlist.IO.toTrack

class Database @VisibleForTesting constructor(private val db: DatabaseDelegate) {
    constructor(ctx: Context) : this(
        Room.databaseBuilder(ctx, DatabaseDelegate::class.java, NAME).build()
    ) {
        DBStatistics.send(db.openHelper)
    }

    fun close() = db.close()

    fun addPlaylist(title: String) = db.playlists().add(PlaylistRecord(title = title)).let {
        Playlist.Id(it)
    }

    fun queryPlaylists(visitor: Consumer<Playlist>) = db.playlists().query().onEach {
        visitor.accept(Playlist(id = it.id, title = it.title))
    }

    interface PlaylistFacade {
        fun exists(): Boolean
        fun addTrack(track: Track.Metadata): Track.Id
        fun addTracks(tracks: Track.IdSet)
        fun queryTracks(): Cursor
        fun queryStatistics(): Track.Statistics
        fun delete()
        fun deleteTracks(tracks: Track.IdSet)
        fun moveTracks(tracks: Track.IdSet, target: Playlist.Id)
        fun sort(spec: Playlist.Sorting)
        fun reorder(track: Track.Id, delta: Int)

        @VisibleForTesting
        fun queryTracks(visitor: Consumer<Track>) = queryTracks().use { cursor ->
            while (cursor.moveToNext()) {
                visitor.accept(cursor.toTrack())
            }
        }
    }

    fun getPlaylist(playlist: Playlist.Id = Playlist.DEFAULT_ID) = object : PlaylistFacade {
        override fun exists() =
            playlist == Playlist.DEFAULT_ID || db.playlists().query(playlist) != null

        override fun addTrack(track: Track.Metadata) =
            db.tracks().add(playlist, TrackRecord(meta = track)).let {
                Track.Id(it)
            }

        override fun addTracks(tracks: Track.IdSet) = db.tracks().add(playlist, tracks)

        override fun queryTracks() = db.tracks().query(playlist)

        override fun queryStatistics() = db.playlists().queryStatistics(playlist).data

        override fun delete() = db.playlists().delete(playlist)

        override fun deleteTracks(tracks: Track.IdSet) = db.tracks().delete(playlist, tracks)

        override fun moveTracks(tracks: Track.IdSet, target: Playlist.Id) =
            db.tracks().changePlaylist(playlist, tracks, target)

        override fun sort(spec: Playlist.Sorting) = db.playlists().sort(playlist, spec)

        override fun reorder(track: Track.Id, delta: Int) =
            db.tracks().reorder(playlist, track, delta)
    }

    fun queryTracks(tracks: Track.IdSet) = db.tracks().query(tracks.storage)

    @VisibleForTesting
    fun queryTracks(tracks: Track.IdSet, visitor: Consumer<Track>) = queryTracks(tracks).use { cursor ->
        while (cursor.moveToNext()) {
            visitor.accept(cursor.toTrack())
        }
    }

    fun queryStatistics(tracks: Track.IdSet) = db.tracks().queryStatistics(tracks.storage).data

    companion object {
        const val NAME = "playlist"
        const val VERSION = 3
    }
}

@Entity(tableName = "tracks")
data class TrackRecord(
    @PrimaryKey(autoGenerate = true) val id: Track.Id = Track.Id(0),
    @Embedded val meta: Track.Metadata,
)

object Converters {
    @TypeConverter
    fun writeLocation(id: Identifier) = id.toString()

    @TypeConverter
    fun readLocation(id: String) = Identifier.parse(id)

    @TypeConverter
    fun writeTimeStamp(ts: TimeStamp) = ts.toMilliseconds()

    @TypeConverter
    fun readTimeStamp(ts: Long) = TimeStamp.fromMilliseconds(ts)

    @TypeConverter
    fun writePlaylistId(id: Playlist.Id) = id.value

    @TypeConverter
    fun readPlaylistId(id: IdType) = Playlist.Id(id)

    @TypeConverter
    fun writeTrackId(id: Track.Id) = id.value

    @TypeConverter
    fun readTrackId(id: IdType) = Track.Id(id)
}

// Do not use foreign keys to playlist - Playlist.DEFAULT_ID has no entity
@Entity(
    tableName = "refs",
    indices = [
        Index(value = ["playlist"]),
    ]
)
data class Reference(
    @PrimaryKey(autoGenerate = true) val position: Long = 0,
    val playlist: Playlist.Id,
    val track: Track.Id,
)

@Entity(tableName = "playlists")
data class PlaylistRecord(
    @PrimaryKey(autoGenerate = true) val id: Playlist.Id = Playlist.DEFAULT_ID,
    val title: String,
)

@Entity
data class StatisticsRecord(@Embedded val data: Track.Statistics)

@Dao
abstract class PlaylistDao {
    @Query("SELECT * FROM playlists")
    abstract fun query(): Array<PlaylistRecord>

    @Query("SELECT * FROM playlists WHERE id = :playlist")
    abstract fun query(playlist: Playlist.Id): PlaylistRecord?

    @Insert
    abstract fun add(playlist: PlaylistRecord): Long

    @Transaction
    open fun delete(id: Playlist.Id) {
        deletePlaylist(id)
        clear(id)
    }

    @Query("DELETE FROM playlists WHERE id = :id")
    protected abstract fun deletePlaylist(id: Playlist.Id)

    @Query("DELETE FROM refs WHERE playlist = :playlist")
    abstract fun clear(playlist: Playlist.Id)

    @Query(
        """
    SELECT COUNT(*) AS count, COUNT(DISTINCT(location)) AS locations, SUM(duration) AS duration
      FROM tracks WHERE id IN (SELECT track FROM refs WHERE playlist = :playlist)
    """
    )
    abstract fun queryStatistics(playlist: Playlist.Id): StatisticsRecord

    @Transaction
    open fun sort(playlist: Playlist.Id, spec: Playlist.Sorting) {
        val temporary = playlist.value.inv()
        execute(
            """
    INSERT INTO refs(playlist, track)
      SELECT ?1, r.track FROM tracks AS t INNER JOIN refs AS r
        ON t.id = r.track AND r.playlist = ?2
        ORDER BY t.${spec.by.field} ${spec.order.sql}, r.position ASC
        """, temporary, playlist.value
        )
        clear(playlist)
        migrate(from = Playlist.Id(temporary), to = playlist)
    }

    @Query("UPDATE refs SET playlist = :to WHERE playlist = :from")
    abstract fun migrate(from: Playlist.Id, to: Playlist.Id)

    open fun execute(query: String, vararg args: Any) = execute(SimpleSQLiteQuery(query, args))

    @RawQuery
    protected abstract fun execute(query: SupportSQLiteQuery): Long
}

@Dao
abstract class TracksDao {
    @Transaction
    open fun add(playlist: Playlist.Id, track: TrackRecord) = add(track).also { id ->
        check(id > 0)
        add(Reference(playlist = playlist, track = Track.Id(id)))
    }

    @Transaction
    open fun add(playlist: Playlist.Id, tracks: Track.IdSet) = tracks.forEach { track ->
        add(Reference(playlist = playlist, track = track))
    }

    @Transaction
    open fun changePlaylist(source: Playlist.Id, tracks: Track.IdSet, target: Playlist.Id) {
        deleteRefs(source, tracks.storage)
        add(target, tracks)
    }

    @Insert
    protected abstract fun add(track: TrackRecord): Long

    @Insert
    protected abstract fun add(ref: Reference)

    @RewriteQueriesToDropUnusedColumns
    @Query(
        """
    SELECT * FROM tracks
     INNER JOIN refs ON tracks.id = refs.track AND refs.playlist = :playlist
     ORDER BY refs.position
            """
    )
    abstract fun query(playlist: Playlist.Id): Cursor

    @Query(
        """
            SELECT * FROM tracks WHERE id IN (:tracks)
        """
    )
    abstract fun query(tracks: IdArrayType): Cursor

    @Query(
        """
    SELECT COUNT(*) AS count, COUNT(DISTINCT(location)) AS locations, SUM(duration) AS duration 
    FROM tracks WHERE id IN (:tracks)
        """
    )
    abstract fun queryStatistics(tracks: IdArrayType): StatisticsRecord

    @Transaction
    open fun delete(playlist: Playlist.Id, tracks: Track.IdSet) {
        deleteRefs(playlist, tracks.storage)
        cleanup()
    }

    @Query("DELETE FROM refs WHERE playlist = :playlist AND track IN (:tracks)")
    protected abstract fun deleteRefs(playlist: Playlist.Id, tracks: IdArrayType)

    open fun reorder(playlist: Playlist.Id, track: Track.Id, delta: Int) {
        SimpleSQLiteQuery(
            """WITH
    playlist AS (SELECT position AS pos, track FROM refs WHERE playlist = ?1),
    anchor AS (SELECT pos FROM playlist WHERE track = ?2),
    range AS (SELECT * FROM playlist WHERE pos * $SIGNOF3 >= (SELECT pos FROM anchor) * $SIGNOF3
      ORDER BY pos * $SIGNOF3 ASC LIMIT abs(?3) + 1),
    source AS (SELECT (SELECT COUNT(*) FROM range WHERE pos < r.pos) AS idx, pos, track FROM range r),
    target AS (SELECT (idx - $SIGNOF3 + (SELECT COUNT(*) FROM range)) % (SELECT COUNT(*) FROM range) AS idx, track FROM source),
    remap AS (SELECT source.pos, ?1, target.track FROM source INNER JOIN target ON source.idx = target.idx)
    REPLACE INTO refs(position, playlist, track) SELECT * FROM remap""",
            arrayOf<Any>(playlist.value, track.value, delta)
        ).also {
            execute(it)
        }
    }

    @Query("DELETE FROM tracks WHERE id NOT IN (SELECT DISTINCT track FROM refs)")
    abstract fun cleanup()

    @RawQuery
    protected abstract fun execute(query: SupportSQLiteQuery): Long

    private companion object {
        const val SIGNOF3 = "max(min(?3, 1), -1)"
    }
}

@VisibleForTesting
open class MigrationTo3(startVersion: Int) : Migration(startVersion, 3) {
    override fun migrate(database: SupportSQLiteDatabase) = with(database) {
        // C&P from DatabaseDelegate_Impl
        execSQL("CREATE TABLE IF NOT EXISTS `tracks` (`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `location` TEXT NOT NULL, `title` TEXT NOT NULL, `author` TEXT NOT NULL, `duration` INTEGER NOT NULL)")
        execSQL("CREATE TABLE IF NOT EXISTS `refs` (`position` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `playlist` INTEGER NOT NULL, `track` INTEGER NOT NULL)")
        execSQL("CREATE INDEX IF NOT EXISTS `index_refs_playlist` ON `refs` (`playlist`)")
        execSQL("CREATE TABLE IF NOT EXISTS `playlists` (`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, `title` TEXT NOT NULL)")
        execSQL("CREATE TABLE IF NOT EXISTS room_master_table (id INTEGER PRIMARY KEY,identity_hash TEXT)")
        execSQL("INSERT OR REPLACE INTO room_master_table (id,identity_hash) VALUES(42, 'be426d5b3725c956cfd82d284c3b249d')")
    }
}

/*
 * Table 'playlist' - all entries
 * '_id', integer primary autoincrement - unique entry identifier
 * 'location', text not null- source data identifier including subpath etc
 * 'type', text not null- chiptune type identifier
 * 'author', text - cached/modified entry's author
 * 'title', text - cached/modified entry's title
 * 'duration', integer not null- cached entry's duration in mS
 * 'tags', text - space separated list of hash prefixed words
 * 'properties', text - comma separated list of name=value entries
 */
@VisibleForTesting
object Migration1to3 : MigrationTo3(1) {
    override fun migrate(database: SupportSQLiteDatabase) {
        super.migrate(database)
        with(database) {
            execSQL("INSERT INTO tracks SELECT _id, location, title, author, duration FROM playlist")
            execSQL("INSERT INTO refs (playlist, track) SELECT 0, _id FROM playlist")
            execSQL("DROP TABLE playlist")
        }
    }
}

/*
 * Table 'tracks' - metadata
 * '_id', integer primary autoincrement - unique entry identifier
 * 'location', text not null - source data identifier including subpath etc
 * 'author', text - cached/modified entry's author
 * 'title', text - cached/modified entry's title
 * 'duration', integer not null- cached entry's duration in mS
 * 'properties', blob - encoded properties
 */

/*
 * Table 'positions' - relative positions
 * 'pos', integer primary autoincrement - unique relative position
 * 'track_id', integer not null - identifier of track
 */

/*
 * Table 'playlist' - virtual table + triggers
 * _id, pos, location, author, title, duration, properties
 * playlist_insert, playlist_delete - triggers
 */
@VisibleForTesting
object Migration2to3 : MigrationTo3(2) {
    override fun migrate(database: SupportSQLiteDatabase) {
        with(database) {
            execSQL("ALTER TABLE tracks RENAME TO told")
        }
        super.migrate(database)
        with(database) {
            execSQL("INSERT INTO tracks SELECT _id, location, title, author, duration FROM told")
            execSQL("INSERT INTO refs (playlist, track) SELECT 0, track_id FROM positions ORDER BY pos")
            execSQL("DROP TABLE told")
            execSQL("DROP TABLE positions")
            execSQL("DROP VIEW playlist")
        }
    }
}

@androidx.room.Database(
    entities = [TrackRecord::class, Reference::class, PlaylistRecord::class],
    version = Database.VERSION,
    exportSchema = false,
)
@TypeConverters(Converters::class)
abstract class DatabaseDelegate : RoomDatabase() {
    abstract fun playlists(): PlaylistDao
    abstract fun tracks(): TracksDao
}
