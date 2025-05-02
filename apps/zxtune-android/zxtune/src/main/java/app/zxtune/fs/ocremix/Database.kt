package app.zxtune.fs.ocremix

import android.content.Context
import androidx.annotation.VisibleForTesting
import androidx.room.ColumnInfo
import androidx.room.Dao
import androidx.room.DatabaseView
import androidx.room.Embedded
import androidx.room.Entity
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.PrimaryKey
import androidx.room.Query
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.room.TypeConverter
import androidx.room.TypeConverters
import app.zxtune.TimeStamp
import app.zxtune.fs.dbhelpers.Timestamps
import app.zxtune.fs.dbhelpers.Utils

/*
 * Version 1 - initial
 * Version 2 - avoid WITH statement with VIEW. Supported only from sqlite 3.8.3 (sdk 21+)
 */

const val NAME = "ocremix"
const val VERSION = 2

class Database @VisibleForTesting constructor(private val db: DatabaseDelegate) {

    constructor(ctx: Context) : this(
        Room.databaseBuilder(ctx, DatabaseDelegate::class.java, NAME)
            .fallbackToDestructiveMigration().build()
    ) {
        Utils.sendStatistics(db.openHelper)
    }

    fun close() = db.close()

    fun runInTransaction(cmd: Utils.ThrowingRunnable) = Utils.runInTransaction(db, cmd)
    fun getLifetime(id: String, ttl: TimeStamp) = db.timestamps().getLifetime(id, ttl)

    fun addSystem(system: System, image: FilePath?) = with(db.catalog()) {
        add(system)
        image?.let {
            addImage(system.id.value, it)
        }
    }

    fun querySystems(visitor: Catalog.SystemsVisitor) =
        db.catalog().querySystems().onEach { visitor.accept(it.system, it.image) }.isNotEmpty()

    fun addOrganization(organization: Organization) = db.catalog().add(organization)

    fun queryOrganizations(visitor: Catalog.Visitor<Organization>) =
        db.catalog().queryOrganizations().onEach { visitor.accept(it) }.isNotEmpty()

    fun addGame(game: Game, system: System, organization: Organization?, image: FilePath?) =
        with(db.catalog()) {
            add(game)
            add(system)
            add(game ownedBy system)
            organization?.let {
                add(it)
                add(game ownedBy it)
            }
            image?.let {
                addImage(game.id.value, it)
            }
            add(game ownedBy ALL_GAMES_SCOPE)
        }

    fun queryGames(scope: Catalog.Scope?, visitor: Catalog.GamesVisitor) =
        db.catalog().queryGames(scope?.id ?: ALL_GAMES_SCOPE)
            .onEach { visitor.accept(it.game, it.system, it.organization, it.image) }.isNotEmpty()

    fun addRemix(scope: Catalog.Scope?, remix: Remix, game: Game) = with(db.catalog()) {
        add(game)
        add(remix)
        add(remix ownedBy game)
        scope?.let {
            add(remix ownedBy it.id)
        }
        add(game ownedBy ALL_GAMES_SCOPE)
        add(remix ownedBy ALL_REMIXES_SCOPE)
    }

    fun queryRemixes(scope: Catalog.Scope?, visitor: Catalog.RemixesVisitor) =
        db.catalog().queryRemixes(scope?.id ?: ALL_REMIXES_SCOPE)
            .onEach { visitor.accept(it.remix, it.game) }.isNotEmpty()

    fun addAlbum(scope: Catalog.Scope?, album: Album, image: FilePath?) = with(db.catalog()) {
        add(album)
        image?.let {
            addImage(album.id.value, it)
        }
        scope?.let {
            add(album ownedBy it.id)
        }
        add(album ownedBy ALL_ALBUMS_SCOPE)
    }

    fun queryAlbums(scope: Catalog.Scope?, visitor: Catalog.AlbumsVisitor) =
        db.catalog().queryAlbums(scope?.id ?: ALL_ALBUMS_SCOPE)
            .onEach { visitor.accept(it.album, it.image) }.isNotEmpty()

    fun addMusicFile(id: String, path: FilePath, size: Long? = null) =
        db.catalog().add(MusicRecord(id, path, size))

    fun deleteMusicFiles(id: String) = db.catalog().deleteMusic(id)
    fun queryMusicFiles(id: String) = db.catalog().queryMusic(id)

    fun addImage(id: String, path: FilePath) = db.catalog().add(ImageRecord(id, path))
    fun queryImage(id: String) = db.catalog().queryImage(id)
}

// TODO: migrate to embedded entity after migration to room 2.6.0

@Entity(tableName = "systems", primaryKeys = ["id"])
class SystemRecord(
    @Embedded val system: System
)

@Entity(tableName = "organizations", primaryKeys = ["id"])
class OrganizationRecord(
    @Embedded val organization: Organization
)

@DatabaseView(
    """
    SELECT scopes.id AS owned,organizations.id AS id,organizations.title AS title FROM scopes 
    INNER JOIN organizations ON scopes.scope=organizations.id""", viewName = "organizations_owned"
)
class OrganizationsOwned(
    @PrimaryKey val owned: String, @Embedded val organization: Organization
)

@Entity(tableName = "games", primaryKeys = ["id"])
class GameRecord(
    @Embedded val game: Game
)

@Entity(tableName = "remixes", primaryKeys = ["id"])
class RemixRecord(
    @Embedded val remix: Remix
)

@Entity(tableName = "albums", primaryKeys = ["id"])
class AlbumRecord(
    @Embedded val album: Album
)

@Entity(tableName = "images")
class ImageRecord(
    @PrimaryKey val id: String,
    val path: FilePath,
)

@Entity(tableName = "music", primaryKeys = ["id", "path"])
class MusicRecord(
    val id: String,
    val path: FilePath,
    val size: Long?,
)

@Entity(tableName = "scopes", primaryKeys = ["id", "scope"])
class ScopeRecord(
    val id: String,
    @ColumnInfo(index = true) val scope: String,
    @ColumnInfo(name = "is_primary", defaultValue = "0") val isPrimary: Boolean = false
)

infix fun Game.ownedBy(scope: System) = ScopeRecord(id.value, scope.id.value)
infix fun Game.ownedBy(scope: Organization) = ScopeRecord(id.value, scope.id.value)
infix fun Game.ownedBy(scope: String) = ScopeRecord(id.value, scope)
infix fun Remix.ownedBy(scope: Game) = ScopeRecord(id.value, scope.id.value, isPrimary = true)
infix fun Remix.ownedBy(scope: String) = ScopeRecord(id.value, scope)
infix fun Album.ownedBy(scope: String) = ScopeRecord(id.value, scope)

val Catalog.Scope.id
    get() = when (this) {
        is Catalog.SystemScope -> id.value
        is Catalog.OrganizationScope -> id.value
        is Catalog.GameScope -> id.value
    }

const val ALL_GAMES_SCOPE = "all_games"
const val ALL_REMIXES_SCOPE = "all_remixes"
const val ALL_ALBUMS_SCOPE = "all_albums"

class QueriedSystem(
    @Embedded val system: System,
    val image: FilePath?,
)

class QueriedGame(
    @Embedded(prefix = "g_") val game: Game,
    @Embedded(prefix = "s_") val system: System,
    @Embedded(prefix = "o_") val organization: Organization?,
    val image: FilePath?,
)

class QueriedRemix(
    @Embedded(prefix = "r_") val remix: Remix,
    @Embedded(prefix = "g_") val game: Game,
)

class QueriedAlbum(
    @Embedded val album: Album,
    val image: FilePath?,
)

data class QueriedMusic(
    val path: FilePath,
    val size: Long?,
)

@Dao
interface CatalogDao {
    @Insert(entity = SystemRecord::class, onConflict = OnConflictStrategy.REPLACE)
    fun add(sys: System)

    @Query("""
    SELECT systems.id AS id,systems.title AS title,images.path as image FROM systems
    LEFT JOIN images USING(id)
    """)
    fun querySystems(): Array<QueriedSystem>

    @Insert(entity = OrganizationRecord::class, onConflict = OnConflictStrategy.REPLACE)
    fun add(org: Organization)

    @Query("SELECT * FROM organizations")
    fun queryOrganizations(): Array<Organization>

    @Insert(entity = GameRecord::class, onConflict = OnConflictStrategy.REPLACE)
    fun add(game: Game)

    @Query(
        """
    SELECT games.id AS g_id,games.title AS g_title,systems.id AS s_id,systems.title AS s_title,
     organizations_owned.id AS o_id,organizations_owned.title AS o_title,images.path AS image
     FROM games 
     INNER JOIN scopes AS g_scopes ON games.id=g_scopes.id AND g_scopes.scope=:scope 
     INNER JOIN scopes AS s_scopes ON games.id=s_scopes.id 
     INNER JOIN systems ON systems.id=s_scopes.scope 
     LEFT JOIN organizations_owned ON games.id=organizations_owned.owned
     LEFT JOIN images ON games.id=images.id
    """
    )
    fun queryGames(scope: String): Array<QueriedGame>

    @Insert(entity = RemixRecord::class, onConflict = OnConflictStrategy.REPLACE)
    fun add(remix: Remix)

    @Query(
        """
    SELECT remixes.id AS r_id,remixes.title AS r_title,games.id AS g_id,games.title AS g_title
     FROM remixes
     INNER JOIN scopes AS r_scopes ON remixes.id=r_scopes.id AND r_scopes.scope=:scope
     INNER JOIN scopes AS g_scopes ON remixes.id=g_scopes.id AND g_scopes.is_primary=1
     INNER JOIN games ON games.id=g_scopes.scope
    """
    )
    fun queryRemixes(scope: String): Array<QueriedRemix>

    @Insert(entity = AlbumRecord::class, onConflict = OnConflictStrategy.REPLACE)
    fun add(album: Album)

    @Query(
        """
        SELECT albums.id AS id,albums.title AS title,images.path as image FROM albums
         INNER JOIN scopes ON albums.id=scopes.id AND scopes.scope=:scope
         LEFT JOIN images USING(id)
    """
    )
    fun queryAlbums(scope: String): Array<QueriedAlbum>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun add(imageRecord: ImageRecord)

    @Query("SELECT path FROM images WHERE id=:id")
    fun queryImage(id: String): FilePath?

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    fun add(musicRecord: MusicRecord)

    @Query("DELETE FROM music WHERE id=:id")
    fun deleteMusic(id: String)

    @Query("SELECT path,size FROM music WHERE id=:id")
    fun queryMusic(id: String): Array<QueriedMusic>

    @Insert(onConflict = OnConflictStrategy.IGNORE) // ignore to avoid isPrimary drop
    fun add(scope: ScopeRecord)
}

object Converters {
    @TypeConverter
    fun writeSystemId(id: System.Id) = id.value

    @TypeConverter
    fun readSystemId(id: String) = System.Id(id)

    @TypeConverter
    fun writeOrganizationId(id: Organization.Id) = id.value

    @TypeConverter
    fun readOrganizationId(id: String) = Organization.Id(id)

    @TypeConverter
    fun writeGameId(id: Game.Id) = id.value

    @TypeConverter
    fun readGameId(id: String) = Game.Id(id)

    @TypeConverter
    fun writeRemixId(id: Remix.Id) = id.value

    @TypeConverter
    fun readRemixId(id: String) = Remix.Id(id)

    @TypeConverter
    fun writeAlbumId(id: Album.Id) = id.value

    @TypeConverter
    fun readAlbumId(id: String) = Album.Id(id)

    @TypeConverter
    fun writePath(path: FilePath) = path.value

    @TypeConverter
    fun readPath(path: String) = FilePath(path)
}

@androidx.room.Database(
    entities = [SystemRecord::class, OrganizationRecord::class, GameRecord::class, RemixRecord::class, AlbumRecord::class, ImageRecord::class, MusicRecord::class, ScopeRecord::class, Timestamps.DAO.Record::class],
    views = [OrganizationsOwned::class],
    version = VERSION
)
@TypeConverters(Converters::class)
abstract class DatabaseDelegate : RoomDatabase() {
    abstract fun catalog(): CatalogDao
    abstract fun timestamps(): Timestamps.DAO
}
