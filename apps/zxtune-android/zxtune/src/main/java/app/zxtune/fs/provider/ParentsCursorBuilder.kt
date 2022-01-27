package app.zxtune.fs.provider

import android.database.Cursor
import android.database.MatrixCursor
import android.net.Uri
import app.zxtune.fs.VfsObject
import java.util.*

internal object ParentsCursorBuilder {
    fun makeParents(obj: VfsObject?, schema: SchemaSource): Cursor {
        val dirs = ArrayList<VfsObject>().apply {
            if (Uri.EMPTY != obj?.uri) {
                var o = obj
                while (o != null) {
                    add(o)
                    o = o.parent
                }
                reverse()
            }
        }
        return MatrixCursor(Schema.Parents.COLUMNS, dirs.size).apply {
            schema.parents(dirs).forEach {
                addRow(Schema.Parents.Object(it.uri, it.name, it.icon).serialize())
            }
        }
    }
}
