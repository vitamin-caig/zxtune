package app.zxtune.fs.vgmrips

import android.content.Context
import androidx.annotation.IntDef
import androidx.annotation.VisibleForTesting
import androidx.room.ColumnInfo
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
import app.zxtune.fs.dbhelpers.Timestamps.DAO
import app.zxtune.fs.dbhelpers.Utils
import app.zxtune.fs.vgmrips.Database.Type

/**
 * Version 1 - initial version from java
 * Version 2 - all fields are not null (track.title, track.duration, track.location in 'tracks' table)
 * Version 3 - added Pack.coverArtLocation
 */

const val NAME = "vgmrips"
const val VERSION = 3

internal open class Database @VisibleForTesting constructor(private val db: DatabaseDelegate) {
    @IntDef(
        TYPE_COMPANY, TYPE_COMPOSER, TYPE_CHIP, TYPE_SYSTEM
    )
    internal annotation class Type
    companion object {
        internal const val TYPE_COMPANY = 0
        internal const val TYPE_COMPOSER = 1
        internal const val TYPE_CHIP = 2
        internal const val TYPE_SYSTEM = 3
    }

    constructor(ctx: Context) : this(
        Room.databaseBuilder(ctx, DatabaseDelegate::class.java, NAME)
            .fallbackToDestructiveMigration().build()
    ) {
        Utils.sendStatistics(db.openHelper)
    }

    fun close() {
        db.close()
    }

    open fun runInTransaction(cmd: Utils.ThrowingRunnable) = Utils.runInTransaction(db, cmd)

    open fun getLifetime(id: String, ttl: TimeStamp) = db.timestamps().getLifetime(id, ttl)

    open fun queryGroups(@Type type: Int, visitor: Catalog.Visitor<Group>) =
        db.catalog().queryGroups(type).onEach(visitor::accept).isNotEmpty()

    open fun addGroup(@Type type: Int, obj: Group) =
        db.catalog().insertGroup(GroupEntity(type, obj))

    open fun queryGroupPacks(id: String, visitor: Catalog.Visitor<Pack>) =
        db.catalog().queryGroupPacks(id).onEach(visitor::accept).isNotEmpty()

    open fun addGroupPack(id: String, obj: Pack) = with(db.catalog()) {
        insertPack(obj)
        bindGroupToPack(GroupPacksRef(id, obj.id))
    }

    open fun addPack(obj: Pack) = db.catalog().insertPack(obj)

    open fun queryPack(id: String) = db.catalog().queryPack(id)

    open fun queryRandomPack() = db.catalog().queryRandomPack()

    open fun queryPackTracks(id: String, visitor: Catalog.Visitor<Track>) =
        db.catalog().queryPackTracks(id).onEach(visitor::accept).isNotEmpty()

    open fun addPackTrack(id: String, obj: Track) = db.catalog().insertTrack(TrackEntity(id, obj))
}

// Groups are queried by type, so type is the first element of key
@Entity(primaryKeys = ["type", "id"], tableName = "groups")
class GroupEntity internal constructor(
    @Type val type: Int, @Embedded val group: Group
)

// Cross-reference entity
@Entity(primaryKeys = ["id", "pack"], tableName = "group_packs")
class GroupPacksRef internal constructor(
    @ColumnInfo(name = "id") val id: String, @ColumnInfo(name = "pack") val packId: String
)

@Entity(primaryKeys = ["id"], tableName = "packs")
class PackEntity internal constructor(
    @Embedded val pack: Pack
)

@Entity(primaryKeys = ["pack_id", "number"], tableName = "tracks")
class TrackEntity internal constructor(
    @ColumnInfo(name = "pack_id") val packId: String, @Embedded val track: Track
)

object TimeStampConverter {
    @TypeConverter
    fun toTimestamp(input: TimeStamp) = input.toSeconds()

    @TypeConverter
    fun fromTimestamp(input: Long) = TimeStamp.fromSeconds(input)
}

@Dao
abstract class CatalogDao {
    @Query("SELECT id, title, packs FROM groups WHERE type = :type")
    abstract fun queryGroups(@Type type: Int): Array<Group>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    abstract fun insertGroup(entity: GroupEntity)

    @Query(
        "SELECT p.id, p.title, p.songs, p.score, p.ratings, p.imageLocation FROM packs AS p, " +
                "group_packs WHERE group_packs.id = :groupId AND p.id = group_packs.pack"
    )
    abstract fun queryGroupPacks(groupId: String): Array<Pack>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    abstract fun bindGroupToPack(ref: GroupPacksRef)

    @Query("SELECT * FROM packs WHERE id = :id")
    abstract fun queryPack(id: String): Pack?

    @Query("SELECT * FROM packs where (SELECT COUNT(*) FROM tracks WHERE id = pack_id) != 0 ORDER BY RANDOM() LIMIT 1")
    abstract fun queryRandomPack(): Pack?

    @Insert(entity = PackEntity::class, onConflict = OnConflictStrategy.REPLACE)
    abstract fun insertPack(pack: Pack)

    @Query("SELECT number, title, duration, location FROM tracks WHERE pack_id = :id")
    abstract fun queryPackTracks(id: String): Array<Track>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    abstract fun insertTrack(track: TrackEntity)
}

@androidx.room.Database(
    entities = [GroupEntity::class, GroupPacksRef::class, PackEntity::class, TrackEntity::class, DAO.Record::class],
    version = VERSION
)
@TypeConverters(TimeStampConverter::class)
abstract class DatabaseDelegate : RoomDatabase() {
    abstract fun catalog(): CatalogDao
    abstract fun timestamps(): DAO
}
