package app.zxtune.fs.khinsider

import android.content.Context
import androidx.annotation.VisibleForTesting
import androidx.core.net.toUri
import androidx.core.util.Consumer
import androidx.room.Dao
import androidx.room.Embedded
import androidx.room.Entity
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.room.TypeConverter
import androidx.room.TypeConverters
import app.zxtune.TimeStamp
import app.zxtune.fs.dbhelpers.DBStatistics
import app.zxtune.fs.dbhelpers.Timestamps
import app.zxtune.fs.dbhelpers.Utils

const val NAME = "khinsider"
const val VERSION = 1

class Database @VisibleForTesting constructor(private val db: DatabaseDelegate) {
    constructor(ctx: Context) : this(
        Room.databaseBuilder(ctx, DatabaseDelegate::class.java, NAME)
            .fallbackToDestructiveMigration().build()
    ) {
        DBStatistics.send(db.openHelper)
    }

    fun close() = db.close()

    fun runInTransaction(cmd: Utils.ThrowingRunnable) = Utils.runInTransaction(db, cmd)
    fun getLifetime(id: String, ttl: TimeStamp) = db.timestamps().getLifetime(id, ttl)

    fun addScope(type: Catalog.ScopeType, scope: Scope) = db.catalog().add(ScopeRecord(type, scope))
    fun queryScopes(type: Catalog.ScopeType, visitor: Consumer<Scope>) =
        db.catalog().queryScopes(type).onEach(visitor::accept).isNotEmpty()

    fun cleanupScope(scope: Scope.Id) = db.catalog().cleanup(scope)
    fun addAlbum(scope: Scope.Id, album: AlbumAndDetails) = with(db.catalog()) {
        // albums from scopes may be not full (e.g. top scopes), so do not overwrite existing
        maybeAdd(AlbumRecord(album))
        add(ScopeAlbum(scope, album.album.id))
    }

    fun addAlbumAlias(album: Album.Id, real: Album.Id) = db.catalog().add(AlbumRecord(album, real))

    fun cleanupAlbum(album: Album.Id) = db.catalog().cleanup(album)
    fun addAlbum(album: AlbumAndDetails) = db.catalog().add(AlbumRecord(album))

    fun queryAlbums(scope: Scope.Id, visitor: Consumer<AlbumAndDetails>) = with(db.catalog()) {
        queryAlbums(scope).onEach { visitor.accept(it.toDetails(this)) }.isNotEmpty()
    }

    fun queryRandomAlbum() = with(db.catalog()) {
        queryRandomAlbum()?.toDetails(this)
    }

    fun queryAlbum(id: Album.Id) = with(db.catalog()) {
        queryAlbum(id)?.toDetails(this)
    }

    fun addTrack(track: TrackAndDetails) = with(db.catalog()) {
        add(
            TrackRecord(
                album = track.album.id,
                track = track.track,
                index = track.index,
                duration = track.duration,
                size = track.size
            )
        )
    }

    fun queryTracks(album: Album, visitor: Consumer<TrackAndDetails>) =
        db.catalog().queryTracks(album.id).onEach {
            visitor.accept(
                TrackAndDetails(
                    album = album,
                    track = it.track,
                    index = it.index,
                    duration = it.duration,
                    size = it.size
                )
            )
        }.isNotEmpty()
}

@Entity(tableName = "scopes", primaryKeys = ["type", "id"])
class ScopeRecord(
    val type: Catalog.ScopeType,
    @Embedded val scope: Scope,
)

@Entity(tableName = "albums", primaryKeys = ["id"])
class AlbumRecord(
    val id: Album.Id,
    val titleOrRealId: String,
    val details: String?,
    val image: FilePath?,
) {
    constructor(album: AlbumAndDetails) : this(
        album.album.id, album.album.title, album.details, album.image
    )

    constructor(album: Album.Id, real: Album.Id) : this(album, real.value, null, null)

    fun toDetails(db: CatalogDao): AlbumAndDetails? = details?.let {
        AlbumAndDetails(Album(id, titleOrRealId), it, image)
    } ?: db.queryAlbum(Album.Id(titleOrRealId))?.toDetails(db)
}

@Entity(tableName = "scope_albums", primaryKeys = ["scope", "album"])
class ScopeAlbum(
    val scope: Scope.Id,
    val album: Album.Id,
)

@Entity(tableName = "tracks", primaryKeys = ["album", "id"])
class TrackRecord(
    val album: Album.Id,
    @Embedded val track: Track,
    val index: Int,
    val duration: String,
    val size: String,
)

@Dao
interface CatalogDao {
    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun add(scope: ScopeRecord)

    @Query("SELECT id, title FROM scopes WHERE type = :type")
    fun queryScopes(type: Catalog.ScopeType): Array<Scope>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun add(album: AlbumRecord)

    @Insert(onConflict = OnConflictStrategy.IGNORE)
    fun maybeAdd(album: AlbumRecord)

    @Query("DELETE FROM scope_albums WHERE scope = :scope")
    fun cleanup(scope: Scope.Id)

    @Query("DELETE FROM tracks WHERE album = :album")
    fun cleanup(album: Album.Id)

    @Insert(onConflict = OnConflictStrategy.IGNORE)
    fun add(bind: ScopeAlbum)

    @Query("SELECT * FROM albums WHERE id IN (SELECT album FROM scope_albums WHERE scope = :scope)")
    fun queryAlbums(scope: Scope.Id): Array<AlbumRecord>

    @Query("SELECT * FROM albums ORDER BY RANDOM() LIMIT 1")
    fun queryRandomAlbum(): AlbumRecord?

    @Query("SELECT * FROM albums WHERE id = :album")
    fun queryAlbum(album: Album.Id): AlbumRecord?

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun add(track: TrackRecord)

    @Query("SELECT * FROM tracks WHERE album = :album")
    fun queryTracks(album: Album.Id): Array<TrackRecord>
}

object Converters {
    @TypeConverter
    fun writeScopeType(type: Catalog.ScopeType) = type.ordinal

    @TypeConverter
    fun readScopeType(type: Int) = Catalog.ScopeType.entries[type]

    @TypeConverter
    fun writeScopeId(id: Scope.Id) = id.value

    @TypeConverter
    fun readScopeId(id: String) = Scope.Id(id)

    @TypeConverter
    fun writeAlbumId(id: Album.Id) = id.value

    @TypeConverter
    fun readAlbumId(id: String) = Album.Id(id)

    @TypeConverter
    fun writeTrackId(id: Track.Id) = id.value

    @TypeConverter
    fun readTrackId(id: String) = Track.Id(id)

    @TypeConverter
    fun writePath(path: FilePath) = path.value.toString()

    @TypeConverter
    fun readPath(path: String) = FilePath(path.toUri())
}

@androidx.room.Database(
    entities = [ScopeRecord::class, AlbumRecord::class, ScopeAlbum::class, TrackRecord::class, Timestamps.DAO.Record::class],
    version = VERSION,
)
@TypeConverters(Converters::class)
abstract class DatabaseDelegate : RoomDatabase() {
    abstract fun catalog(): CatalogDao
    abstract fun timestamps(): Timestamps.DAO
}
