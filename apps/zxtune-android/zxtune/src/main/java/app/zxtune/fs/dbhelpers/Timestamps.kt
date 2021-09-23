/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.dbhelpers

import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteDoneException
import android.database.sqlite.SQLiteStatement
import androidx.annotation.VisibleForTesting
import androidx.room.Dao
import androidx.room.Entity
import androidx.room.PrimaryKey
import androidx.room.Query
import app.zxtune.TimeStamp

class Timestamps @VisibleForTesting internal constructor(
    readable: SQLiteDatabase,
    writable: SQLiteDatabase
) {
    private object Table {
        const val CREATE_QUERY = "CREATE TABLE timestamps (" +
                "_id  TEXT PRIMARY KEY, " +
                "stamp DATETIME NOT NULL" +
                ");"
        const val QUERY_STATEMENT =
            "SELECT strftime('%s', 'now') - strftime('%s', stamp) FROM timestamps WHERE _id = ?;"
        const val INSERT_STATEMENT = "REPLACE INTO timestamps VALUES (?, CURRENT_TIMESTAMP);"
    }

    companion object {
        const val CREATE_QUERY = Table.CREATE_QUERY
    }

    interface Lifetime {
        val isExpired: Boolean

        fun update()
    }

    private val queryStatement: SQLiteStatement = readable.compileStatement(Table.QUERY_STATEMENT)
    private val updateStatement: SQLiteStatement = writable.compileStatement(Table.INSERT_STATEMENT)

    constructor(helper: DBProvider) : this(helper.readableDatabase, helper.writableDatabase) {}

    fun getLifetime(id: String, ttl: TimeStamp): Lifetime = DbLifetime(id, ttl)

    private fun queryAge(id: String) = synchronized(queryStatement) {
        queryStatement.clearBindings()
        queryStatement.bindString(1, id)
        queryStatement.simpleQueryForLong()
    }

    private fun updateAge(id: String): Unit = synchronized(updateStatement) {
        updateStatement.clearBindings()
        updateStatement.bindString(1, id)
        updateStatement.executeInsert()
    }

    private inner class DbLifetime(
        private val objId: String,
        private val TTL: TimeStamp
    ) : Lifetime {
        override val isExpired
            get() = try {
                TimeStamp.fromSeconds(queryAge(objId)) > TTL
            } catch (e: SQLiteDoneException) {
                true
            }

        override fun update() = updateAge(objId)
    }

    // TODO: make the only implementation
    @Dao
    abstract class DAO {
        @Entity(tableName = "timestamps")
        class Record {
            @PrimaryKey
            var id: String = ""
            var stamp: Long = 0
        }

        @Query("SELECT strftime('%s') - stamp FROM timestamps WHERE id = :id")
        protected abstract fun queryAge(id: String): Long?

        @Query("REPLACE INTO timestamps VALUES (:id, strftime('%s'))")
        protected abstract fun touch(id: String)

        private inner class LifetimeImpl(private val objId: String, private val TTL: TimeStamp) :
            Lifetime {
            override val isExpired
                get() = queryAge(objId)?.let { TimeStamp.fromSeconds(it) > TTL } ?: true

            override fun update() = touch(objId)
        }

        fun getLifetime(id: String, ttl: TimeStamp): Lifetime = LifetimeImpl(id, ttl)
    }
}
