/**
 * @file
 * @brief DAL helper
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.dbhelpers

import android.content.Context
import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper
import app.zxtune.Logger
import app.zxtune.TimeStamp
import java.io.*

/**
 * Version 1
 *
 * CREATE TABLE dirs (_id TEXT PRIMARY KEY, entries BLOB NOT NULL);
 * standard timestamps
 *
 * entries - serialized HashMap<String></String>, String>
 *
 * Version 2
 *
 * Changed serialization format of blob
 */

private val LOG = Logger(FileTree::class.java.name)

private const val VERSION = 2

open class FileTree(context: Context, id: String) {

    private val helper = DBProvider(Helper(context, id))
    private val entries = Table(helper)
    private val timestamps = Timestamps(helper)

    fun close() = helper.close()

    @Throws(IOException::class)
    open fun runInTransaction(cmd: Utils.ThrowingRunnable) = Utils.runInTransaction(helper, cmd)

    open fun getDirLifetime(path: String, ttl: TimeStamp) = timestamps.getLifetime(path, ttl)

    class Entry(val name: String, val descr: String, sizeParam: String?) {

        val size = sizeParam.orEmpty()

        constructor(input: DataInput) : this(
            input.readUTF(),
            input.readUTF(),
            input.readUTF()
        )

        fun save(out: DataOutput) {
            out.writeUTF(name)
            out.writeUTF(descr)
            out.writeUTF(size)
        }

        val isDir
            get() = size.isEmpty()

        override fun equals(other: Any?) = (other as? Entry)?.let { rh ->
            name == rh.name && descr == rh.descr && size == rh.size
        } ?: false
    }

    @Throws(IOException::class)
    open fun add(path: String, obj: List<Entry>) = entries.add(path, serialize(obj))

    open fun find(path: String) = try {
        entries[path]?.let {
            deserialize(it)
        }
    } catch (e: IOException) {
        null
    }
}

private class Helper constructor(context: Context, name: String) :
    SQLiteOpenHelper(context, name, null, VERSION) {
    override fun onCreate(db: SQLiteDatabase) {
        LOG.d { "Creating database" }
        db.execSQL(Table.CREATE_QUERY)
        db.execSQL(Timestamps.CREATE_QUERY)
    }

    override fun onUpgrade(db: SQLiteDatabase, oldVersion: Int, newVersion: Int) {
        LOG.d { "Upgrading database $oldVersion -> $newVersion" }
        Utils.cleanupDb(db)
        onCreate(db)
    }
}

class Table(helper: DBProvider) {
    private val db = helper.writableDatabase
    private val insertStatement = db.compileStatement("INSERT OR REPLACE INTO dirs VALUES(?, ?);")

    fun add(id: String, entries: ByteArray) {
        insertStatement.bindString(1, id)
        insertStatement.bindBlob(2, entries)
        insertStatement.executeInsert()
    }

    operator fun get(id: String) =
        db.query("dirs", arrayOf("entries"), "_id = ?", arrayOf(id), null, null, null)
            ?.use { cursor ->
                if (cursor.moveToFirst()) cursor.getBlob(0) else null
            }

    companion object {
        const val CREATE_QUERY =
            "CREATE TABLE dirs (_id TEXT PRIMARY KEY, entries BLOB NOT NULL);"
    }
}

private fun serialize(obj: List<FileTree.Entry>) = ByteArrayOutputStream(1024).apply {
    DataOutputStream(this).use { out ->
        out.writeInt(obj.size)
        obj.forEach { element -> element.save(out) }
    }
}.toByteArray()

private fun deserialize(data: ByteArray) = ByteArrayInputStream(data).run {
    DataInputStream(this).use { input ->
        val count = input.readInt()
        ArrayList<FileTree.Entry>(count).apply {
            repeat(count) {
                add(FileTree.Entry(input))
            }
        }
    }
}
