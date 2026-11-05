package app.zxtune.fs.sndh

import android.content.Context
import androidx.annotation.VisibleForTesting
import androidx.core.util.Consumer
import androidx.room.Dao
import androidx.room.Entity
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Room
import androidx.room.RoomDatabase
import app.zxtune.TimeStamp
import app.zxtune.fs.dbhelpers.DBStatistics
import app.zxtune.fs.dbhelpers.TimestampRecord
import app.zxtune.fs.dbhelpers.Timestamps
import app.zxtune.fs.dbhelpers.Utils

const val NAME = "sndh"
const val VERSION = 1

class Database @VisibleForTesting constructor(private val db: SndhDatabaseDelegate) {
    constructor(ctx: Context) : this(
        Room.databaseBuilder(ctx, SndhDatabaseDelegate::class.java, NAME)
            .fallbackToDestructiveMigration().build()
    ) {
        DBStatistics.send(db.openHelper)
    }

    fun close() = db.close()

    fun runInTransaction(cmd: Utils.ThrowingRunnable) = Utils.runInTransaction(db, cmd)
    fun getLifetime(id: String, ttl: TimeStamp) = db.timestamps().getLifetime(id, ttl)

    fun addAuthor(author: Author) = db.catalog().add(AuthorRecord(author))

    fun cleanupAuthors() = db.catalog().cleanupAuthors()

    fun queryAuthors(visitor: Consumer<Author>) = db.catalog().queryAuthors()
        .onEach { visitor.accept(it.toAuthor()) }.isNotEmpty()

    fun cleanupTracks(author: Author) = db.catalog().cleanupTracks(author.name)

    fun addTrack(author: Author, track: Track) = db.catalog().add(TrackRecord(author, track))

    fun queryTracks(author: Author, visitor: Consumer<Track>) = db.catalog()
        .queryTracks(author.name).onEach { visitor.accept(it.toTrack()) }.isNotEmpty()

    fun findTracks(query: String, visitor: Catalog.FoundTracksVisitor) = db.catalog()
        .findTracks(query).forEach { visitor.accept(Author(it.author), it.toTrack()) }

    fun queryRandomTrack() = db.catalog().queryRandomTrack()?.toTrack()
}

@Entity(tableName = "authors", primaryKeys = ["name"])
class AuthorRecord(
    val name: String,
    val files: Int,
) {
    constructor(author: Author) : this(author.name, author.files)

    fun toAuthor() = Author(name, files)
}

@Entity(tableName = "tracks", primaryKeys = ["id"])
class TrackRecord(
    val id: Int,
    val author: String,
    val title: String,
    val sizeText: String,
) {
    constructor(author: Author, track: Track) : this(
        track.id,
        author.name,
        track.title,
        track.sizeText
    )

    fun toTrack() = Track(id, title, sizeText)
}

@Dao
interface CatalogDao {
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun add(author: AuthorRecord)

    @Query("DELETE FROM authors")
    fun cleanupAuthors()

    @Query("SELECT * FROM authors ORDER BY name")
    fun queryAuthors(): Array<AuthorRecord>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun add(track: TrackRecord)

    @Query("DELETE FROM tracks WHERE author = :author")
    fun cleanupTracks(author: String)

    @Query("SELECT * FROM tracks WHERE author = :author ORDER BY title")
    fun queryTracks(author: String): Array<TrackRecord>

    @Query("SELECT * FROM tracks WHERE title LIKE '%' || :query || '%' ORDER BY title")
    fun findTracks(query: String): Array<TrackRecord>

    @Query("SELECT * FROM tracks ORDER BY RANDOM() LIMIT 1")
    fun queryRandomTrack(): TrackRecord?
}

@androidx.room.Database(
    entities = [AuthorRecord::class, TrackRecord::class, TimestampRecord::class],
    version = VERSION,
)
abstract class SndhDatabaseDelegate : RoomDatabase() {
    abstract fun catalog(): CatalogDao
    abstract fun timestamps(): Timestamps.DAO
}
