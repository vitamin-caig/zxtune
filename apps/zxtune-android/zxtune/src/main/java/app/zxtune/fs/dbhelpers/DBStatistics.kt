package app.zxtune.fs.dbhelpers

import android.database.sqlite.SQLiteOpenHelper
import androidx.sqlite.db.SupportSQLiteOpenHelper
import kotlinx.coroutines.DelicateCoroutinesApi
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.launch

object DBStatistics {
    @JvmStatic
    fun send(helper: SQLiteOpenHelper) = asynchronous {
        Utils.sendStatistics(helper)
    }

    fun send(helper: SupportSQLiteOpenHelper) = asynchronous {
        Utils.sendStatistics(helper)
    }

    @OptIn(DelicateCoroutinesApi::class)
    private fun asynchronous(cmd: () -> Unit) = GlobalScope.launch(Dispatchers.IO) {
        cmd()
    }
}
