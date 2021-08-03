package app.zxtune.fs.provider

import android.database.Cursor
import java.util.concurrent.Callable

internal interface AsyncQueryOperation : Callable<Cursor?> {
    fun status(): Cursor?
}
