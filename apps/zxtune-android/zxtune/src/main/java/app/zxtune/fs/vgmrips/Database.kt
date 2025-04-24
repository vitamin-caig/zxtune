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
import app.zxtune.fs.dbhelpers.Timestamps
import app.zxtune.fs.dbhelpers.Utils

/**
 * Version 1 - initial version from java
 * Version 2 - all fields are not null (track.title, track.duration, track.location in 'tracks' table)
 * Version 3 - added Pack.coverArtLocation
 * Version 4 - added pack location, removed score and rating
 */

const val NAME = "vgmrips"
const val VERSION = 4

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

    open fun queryGroupPacks(id: Group.Id, visitor: Catalog.Visitor<Pack>) =
        db.catalog().queryGroupPacks(id).onEach(visitor::accept).isNotEmpty()

    open fun addGroupPack(id: Group.Id, obj: Pack) = with(db.catalog()) {
        insertPack(obj)
        bindGroupToPack(GroupPacksRef(id, obj.id))
    }

    open fun addPack(obj: Pack) = db.catalog().insertPack(obj)

    open fun queryPack(id: Pack.Id) = db.catalog().queryPack(id)

    open fun queryRandomPack() = db.catalog().queryRandomPack()

    open fun queryPackTracks(id: Pack.Id, visitor: Catalog.Visitor<FilePath>) =
        db.catalog().queryPackTracks(id).onEach(visitor::accept).isNotEmpty()

    open fun addPackTrack(id: Pack.Id, obj: FilePath) =
        db.catalog().insertTrack(TrackEntity(id, obj))
}

// Groups are queried by type, so type is the first element of key
@Entity(primaryKeys = ["type", "id"], tableName = "groups")
class GroupEntity internal constructor(
    @Database.Type val type: Int, @Embedded val group: Group
)

// Cross-reference entity
@Entity(primaryKeys = ["id", "pack"], tableName = "group_packs")
class GroupPacksRef internal constructor(
    @ColumnInfo(name = "id") val id: Group.Id, @ColumnInfo(name = "pack") val packId: Pack.Id
)

@Entity(primaryKeys = ["id"], tableName = "packs")
class PackEntity internal constructor(
    @Embedded val pack: Pack
)

@Entity(primaryKeys = ["pack_id", "track"], tableName = "tracks")
class TrackEntity internal constructor(
    @ColumnInfo(name = "pack_id") val packId: Pack.Id, val track: FilePath
)

object Converters {
    @TypeConverter
    fun toTimestamp(input: TimeStamp) = input.toSeconds()

    @TypeConverter
    fun fromTimestamp(input: Long) = TimeStamp.fromSeconds(input)

    @TypeConverter
    fun writePackId(id: Pack.Id) = id.value

    @TypeConverter
    fun readPackId(id: String) = Pack.Id(id)

    @TypeConverter
    fun writeGroupId(id: Group.Id) = id.value

    @TypeConverter
    fun readGroupId(id: String) = Group.Id(id)

    @TypeConverter
    fun writePath(path: FilePath) = path.value

    @TypeConverter
    fun readPath(path: String) = FilePath(path)
}

@Dao
abstract class CatalogDao {
    @Query("SELECT id, title, packs, image FROM groups WHERE type = :type")
    abstract fun queryGroups(@Database.Type type: Int): Array<Group>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    abstract fun insertGroup(entity: GroupEntity)

    @Query(
        """
        SELECT * FROM packs WHERE id IN (SELECT pack FROM group_packs WHERE id = :groupId) 
        """
    )
    abstract fun queryGroupPacks(groupId: Group.Id): Array<Pack>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    abstract fun bindGroupToPack(ref: GroupPacksRef)

    @Query("SELECT * FROM packs WHERE id = :id")
    abstract fun queryPack(id: Pack.Id): Pack?

    @Query("SELECT * FROM packs where (SELECT COUNT(*) FROM tracks WHERE id = pack_id) != 0 ORDER BY RANDOM() LIMIT 1")
    abstract fun queryRandomPack(): Pack?

    @Insert(entity = PackEntity::class, onConflict = OnConflictStrategy.REPLACE)
    abstract fun insertPack(pack: Pack)

    @Query("SELECT track FROM tracks WHERE pack_id = :id")
    abstract fun queryPackTracks(id: Pack.Id): Array<FilePath>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    abstract fun insertTrack(track: TrackEntity)
}

@androidx.room.Database(
    entities = [GroupEntity::class, GroupPacksRef::class, PackEntity::class, TrackEntity::class, Timestamps.DAO.Record::class],
    version = VERSION
)
@TypeConverters(Converters::class)
abstract class DatabaseDelegate : RoomDatabase() {
    abstract fun catalog(): CatalogDao
    abstract fun timestamps(): Timestamps.DAO
}
