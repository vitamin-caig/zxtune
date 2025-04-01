package app.zxtune.coverart

import android.content.Context
import android.net.Uri
import androidx.annotation.VisibleForTesting
import androidx.core.net.toUri
import androidx.room.ColumnInfo
import androidx.room.Dao
import androidx.room.Embedded
import androidx.room.Entity
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.PrimaryKey
import androidx.room.Query
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.room.Transaction
import androidx.room.TypeConverter
import androidx.room.TypeConverters
import androidx.sqlite.db.SupportSQLiteDatabase
import app.zxtune.core.Identifier
import app.zxtune.fs.dbhelpers.Utils
import java.util.zip.CRC32

/*
 * Version 1 - initial
 * Version 2 - reset cache (no albumart outside of archive, limit archived pictures size)
 * Version 3 - reset cache (filter out DCIM files)
 * Version 4 - split references and blobs, add type
 */

const val NAME = "coverart"
const val VERSION = 4

class Database @VisibleForTesting constructor(private val db: DatabaseDelegate) {
    constructor(ctx: Context) : this(
        Room.databaseBuilder(ctx, DatabaseDelegate::class.java, NAME)
            .fallbackToDestructiveMigration().addCallback(object : RoomDatabase.Callback() {
                override fun onDestructiveMigration(db: SupportSQLiteDatabase) {
                    db.execSQL("DROP TABLE IF EXISTS images")
                }
            }).build()
    ) {
        Utils.sendStatistics(db.openHelper)
    }

    fun close() = db.close()

    fun findEmbedded(id: Identifier) = db.dao()
        .queryReference(id.dataLocation, id.subPath, Reference.DIRECT_PIC, Reference.DIRECT_PIC)

    fun queryBlob(id: Long) = db.dao().queryBlob(id)

    fun queryImageReferences(id: Identifier) = db.dao().queryReference(
        id.dataLocation, id.subPath, Reference.DIRECT_PIC, Reference.INFERRED_PIC
    )

    fun queryImage(id: Identifier) = db.dao().queryPicOrUrl(
        id.dataLocation, id.subPath, Reference.DIRECT_PIC, Reference.INFERRED_PIC
    )

    fun addImage(id: Identifier, blob: ByteArray) = db.dao().add(id, Reference.DIRECT_PIC, blob)
    fun addBoundImage(id: Identifier, uri: Uri) = db.dao().add(id, Reference.BOUND_PIC, uri)
    fun addInferred(id: Identifier, uri: Uri) = db.dao().add(id, Reference.INFERRED_PIC, uri)

    fun setNoImage(id: Identifier) = db.dao().setNoImage(id)

    fun listArchiveImages(archive: Uri) = db.dao().queryImages(archive)

    fun remove(archive: Uri) = db.dao().delete(archive)
}

@Entity(tableName = "pics")
data class Picture(
    @PrimaryKey val id: Long,
    @ColumnInfo(typeAffinity = ColumnInfo.BLOB) val data: ByteArray,
) {
    constructor(data: ByteArray) : this(ImageHash.of(data), data)
}

@Entity(
    tableName = "refs",
    primaryKeys = ["path", "sub_path", "type"],
)
data class Reference(
    val path: Uri,
    @ColumnInfo(name = "sub_path") val subPath: String,
    val type: Int,
    @Embedded val target: Target,
) {
    companion object {
        internal const val DIRECT_PIC = 0
        internal const val BOUND_PIC = 1
        internal const val INFERRED_PIC = 2
    }

    data class Target(
        val pic: Long?,
        val url: Uri?,
    ) {
        constructor(pic: Long) : this(pic, null)
        constructor(url: Uri) : this(null, url)
    }

    constructor(id: Identifier, flags: Int, uri: Uri?) : this(
        id.dataLocation, id.subPath, flags, Target(null, uri)
    )

    constructor(id: Identifier, flags: Int, pic: Long) : this(
        id.dataLocation, id.subPath, flags, Target(pic, null)
    )
}

data class PicOrUrl(
    @Embedded val pic: Picture?,
    val url: Uri?,
) {
    constructor(blob: ByteArray) : this(Picture(blob), null)
    constructor(url: Uri) : this(null, url)
}

object UriConverter {
    @TypeConverter
    fun toString(input: Uri) = input.toString()

    @TypeConverter
    fun fromString(input: String): Uri = input.toUri()
}

@Dao
abstract class CoverArtDao {

    @Query("SELECT data FROM pics WHERE id = :id")
    abstract fun queryBlob(id: Long): ByteArray?

    @Query(
        """
        SELECT pic, url FROM refs WHERE path = :path AND sub_path = :subPath 
         AND type BETWEEN :minType AND :maxType ORDER BY type LIMIT 1
        """
    )
    abstract fun queryReference(
        path: Uri, subPath: String, minType: Int, maxType: Int
    ): Reference.Target?

    @Query(
        """
        SELECT id, data, url FROM refs LEFT JOIN pics ON pic = id 
         WHERE path = :path AND sub_path = :subPath AND type BETWEEN :minType AND :maxType
         ORDER BY type LIMIT 1
        """
    )
    abstract fun queryPicOrUrl(path: Uri, subPath: String, minType: Int, maxType: Int): PicOrUrl?

    @Query("SELECT sub_path FROM refs WHERE path = :location AND pic IS NOT NULL")
    abstract fun queryImages(location: Uri): List<String>

    @Transaction
    open fun add(id: Identifier, type: Int, blob: ByteArray) = Picture(blob).also {
        add(it)
        add(Reference(id, type, it.id))
    }

    open fun add(id: Identifier, flags: Int, url: Uri) = add(Reference(id, flags, url))

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    abstract fun add(ref: Reference)

    @Insert(onConflict = OnConflictStrategy.IGNORE)
    abstract fun add(pic: Picture)

    @Transaction
    open fun setNoImage(id: Identifier) {
        add(Reference(id, Reference.DIRECT_PIC, null))
    }

    @Transaction
    open fun delete(location: Uri) {
        deletePicsOf(location)
        deleteRef(location)
    }

    @Query("DELETE FROM pics WHERE id IN (SELECT pic FROM refs WHERE path = :location)")
    abstract fun deletePicsOf(location: Uri)

    @Query("DELETE FROM refs WHERE path = :location")
    abstract fun deleteRef(location: Uri)
}

@androidx.room.Database(
    entities = [Reference::class, Picture::class],
    version = VERSION,
    exportSchema = false,
)
@TypeConverters(UriConverter::class)
abstract class DatabaseDelegate : RoomDatabase() {
    abstract fun dao(): CoverArtDao
}

@VisibleForTesting
object ImageHash {
    fun of(data: ByteArray) = CRC32().apply {
        update(data, 0, (data.size + 1) / 2)
    }.value xor (CRC32().apply {
        val start = data.size / 2
        update(data, start, data.size - start)
    }.value shl 31)
}
