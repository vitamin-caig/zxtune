package app.zxtune.fs.dbhelpers

import android.database.sqlite.SQLiteDatabase
import android.database.sqlite.SQLiteOpenHelper

class DBProvider(private val delegate: SQLiteOpenHelper) {

    init {
        DBStatistics.send(delegate)
    }

    fun close() = delegate.close()

    val writableDatabase: SQLiteDatabase
        get() = delegate.writableDatabase

    val readableDatabase: SQLiteDatabase
        get() = delegate.readableDatabase
}
