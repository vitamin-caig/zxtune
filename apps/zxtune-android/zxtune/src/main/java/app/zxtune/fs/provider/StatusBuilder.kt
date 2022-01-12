package app.zxtune.fs.provider

import android.database.Cursor
import android.database.MatrixCursor

internal object StatusBuilder {
    @JvmStatic
    fun makeError(e: Throwable): Cursor = MatrixCursor(Schema.Status.COLUMNS, 1).apply {
        addRow(Schema.Status.Error(e).serialize())
    }

    @JvmStatic
    fun makeProgress(done: Int, total: Int): Cursor = MatrixCursor(Schema.Status.COLUMNS, 1).apply {
        addRow(Schema.Status.Progress(done, total).serialize())
    }

    fun makeIntermediateProgress(): Cursor = MatrixCursor(Schema.Status.COLUMNS, 1).apply {
        addRow(Schema.Status.Progress.createIntermediate().serialize())
    }
}
