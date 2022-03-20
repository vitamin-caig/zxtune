package app.zxtune.fs.dbhelpers

import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper
import androidx.room.Database
import androidx.room.Room
import androidx.room.RoomDatabase
import androidx.test.core.app.ApplicationProvider
import app.zxtune.TimeStamp
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class TimestampsTest {

    @Test
    fun `test old timestamps`() = InMemoryDatabase().let { db ->
        val underTest = Timestamps(db.readableDatabase, db.writableDatabase)
        testTimestamp { id, ttl ->
            underTest.getLifetime(id, ttl)
        }
        db.close()
    }

    @Test
    fun `test room timestamps`() = Room.inMemoryDatabaseBuilder(
        ApplicationProvider.getApplicationContext(),
        TimestampsTestDatabase::class.java
    ).allowMainThreadQueries().build().let { db ->
        val underTest = db.timestamps()
        testTimestamp { id, ttl ->
            underTest.getLifetime(id, ttl)
        }
        db.close()
    }
}

private class InMemoryDatabase : SQLiteOpenHelper(null, null, null, 1) {
    override fun onCreate(db: SQLiteDatabase) {
        db.execSQL(Timestamps.CREATE_QUERY)
    }

    override fun onUpgrade(db: SQLiteDatabase?, oldVersion: Int, newVersion: Int) = Unit
}

@Database(entities = [Timestamps.DAO.Record::class], version = 1)
abstract class TimestampsTestDatabase : RoomDatabase() {
    abstract fun timestamps(): Timestamps.DAO
}

private fun testTimestamp(create: (String, TimeStamp) -> Timestamps.Lifetime) {
    val id1 = "id"
    val ttl1 = TimeStamp.fromSeconds(1)
    val ts1 = create(id1, ttl1)
    ts1.run {
        assertTrue(isExpired)
        update()
        assertFalse(isExpired)
    }

    val id2 = "another id"
    val ttl2 = TimeStamp.fromDays(10)
    create(id2, ttl2).run {
        assertTrue(isExpired)
        update()
        assertFalse(isExpired)
    }

    Thread.sleep(2 * ttl1.toMilliseconds())

    assertTrue(ts1.isExpired)
    // check for another instance
    create(id1, ttl1).run {
        assertTrue(isExpired)
    }
    create(id1, ttl2).run {
        assertFalse(isExpired)
    }
    ts1.update()
    assertFalse(ts1.isExpired)
    // another instance
    create(id1, ttl1).run {
        assertFalse(isExpired)
    }

    create(id2, ttl2).run {
        assertFalse(isExpired)
    }
}
