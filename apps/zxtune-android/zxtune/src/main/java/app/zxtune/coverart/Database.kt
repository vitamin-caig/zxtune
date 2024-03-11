package app.zxtune.coverart

import android.content.Context
import android.net.Uri
import androidx.annotation.VisibleForTesting
import androidx.room.ColumnInfo
import androidx.room.Dao
import androidx.room.Entity
import androidx.room.Insert
import androidx.room.OnConflictStrategy
import androidx.room.Query
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.room.TypeConverter
import androidx.room.TypeConverters
import app.zxtune.core.Identifier
import app.zxtune.fs.dbhelpers.Utils

const val NAME = "coverart"
const val VERSION = 1

class Database @VisibleForTesting constructor(private val db: DatabaseDelegate) {
    constructor(ctx: Context) : this(
        Room.databaseBuilder(ctx, DatabaseDelegate::class.java, NAME)
            .fallbackToDestructiveMigration().build()
    ) {
        Utils.sendStatistics(db.openHelper)
    }

    fun hasEmbedded(id: Identifier) = db.dao().hasEmbedded(id.dataLocation, id.subPath)

    fun query(id: Identifier) = db.dao().query(id.dataLocation, id.subPath)

    fun addEmbedded(id: Identifier, blob: ByteArray) =
        db.dao().add(Reference(id.dataLocation, id.subPath, embeddedPicture = blob))

    fun addExternal(id: Identifier, uri: Uri) =
        db.dao().add(Reference(id.dataLocation, id.subPath, externalPicture = uri))

    fun setArchiveImage(archive: Uri, image: Uri? = null) =
        db.dao().add(Reference(archive, "", externalPicture = image))

    fun listArchiveImages(archive: Uri) = db.dao().queryImages(archive)

    fun remove(archive: Uri) = db.dao().delete(archive)
}

@Entity(primaryKeys = ["location", "sub_path"], tableName = "images")
data class Reference(
    val location: Uri,
    @ColumnInfo(name = "sub_path")
    val subPath: String,
    @ColumnInfo(name = "external")
    val externalPicture: Uri? = null,
    @ColumnInfo(name = "embedded", typeAffinity = ColumnInfo.BLOB)
    val embeddedPicture: ByteArray? = null,
)

object UriConverter {
    @TypeConverter
    fun toString(input: Uri) = input.toString()

    @TypeConverter
    fun fromString(input: String): Uri = Uri.parse(input)
}

@Dao
abstract class CoverArtDao {

    @Query("SELECT COUNT(*) FROM images WHERE location = :location and sub_path = :subPath and embedded NOT NULL")
    abstract fun hasEmbedded(location: Uri, subPath: String): Boolean

    @Query("SELECT * FROM images WHERE location = :location AND sub_path = :subPath")
    abstract fun query(location: Uri, subPath: String): Reference?

    @Query("SELECT sub_path FROM images WHERE location = :location AND embedded NOT NULL")
    abstract fun queryImages(location: Uri): List<String>

    @Insert(onConflict = OnConflictStrategy.REPLACE)
    abstract fun add(ref: Reference)

    @Query("DELETE FROM images WHERE location = :location")
    abstract fun delete(location: Uri)
}

@androidx.room.Database(
    entities = [Reference::class], version = VERSION, exportSchema = false,
)
@TypeConverters(UriConverter::class)
abstract class DatabaseDelegate : RoomDatabase() {
    abstract fun dao(): CoverArtDao
}
