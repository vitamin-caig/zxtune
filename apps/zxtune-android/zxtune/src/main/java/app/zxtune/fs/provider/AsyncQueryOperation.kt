package app.zxtune.fs.provider

import android.database.Cursor

internal interface AsyncQueryOperation {
    fun call(): Cursor?
    fun status(): Cursor?

    interface Callback {
        fun checkForCancel()
        fun onStatusChanged()
    }
}
