package app.zxtune.fs.provider;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.ArrayList;
import java.util.Collections;

import app.zxtune.fs.VfsObject;
import app.zxtune.fs.VfsUtils;

class ParentsCursorBuilder {

  static Cursor makeParents(@Nullable VfsObject obj, SchemaSource schema) {
    final ArrayList<VfsObject> entries = new ArrayList<>();
    if (obj != null && !Uri.EMPTY.equals(obj.getUri())) {
      for (VfsObject o = obj; o != null; o = o.getParent()) {
        entries.add(o);
      }
      Collections.reverse(entries);
    }
    final MatrixCursor result = new MatrixCursor(Schema.Parents.COLUMNS, entries.size());
    for (Schema.Parents.Object o : schema.parents(entries)) {
      result.addRow(o.serialize());
    }
    return result;
  }
}
