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

  static Cursor makeParents(@Nullable VfsObject obj) {
    final ArrayList<VfsObject> entries = new ArrayList<>();
    if (obj != null && !Uri.EMPTY.equals(obj.getUri())) {
      for (VfsObject o = obj; o != null; o = o.getParent()) {
        entries.add(o);
      }
      Collections.reverse(entries);
    }
    return makeCursor(entries);
  }

  private static Cursor makeCursor(ArrayList<VfsObject> objects) {
    final MatrixCursor result = new MatrixCursor(Schema.Parents.COLUMNS, objects.size());
    for (VfsObject o : objects) {
      result.addRow(Schema.Parents.make(o.getUri(), o.getName(), VfsUtils.getObjectIcon(o)));
    }
    return result;
  }
}
