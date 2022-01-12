package app.zxtune.fs.provider;

import android.database.Cursor;
import android.database.MatrixCursor;

import androidx.annotation.Nullable;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.concurrent.CancellationException;

import app.zxtune.fs.DefaultComparator;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

class ListingCursorBuilder extends VfsDir.Visitor {

  private final SchemaSource schema;
  private final ArrayList<VfsDir> dirs = new ArrayList<>();
  private final ArrayList<VfsFile> files = new ArrayList<>();
  private int total = 0;
  private int done = 0;

  ListingCursorBuilder(SchemaSource schema) {
    this.schema = schema;
  }

  @Override
  public void onItemsCount(int count) {
    dirs.ensureCapacity(count);
    files.ensureCapacity(count);
  }

  @Override
  public void onDir(VfsDir dir) {
    dirs.add(dir);
  }

  @Override
  public void onFile(VfsFile file) {
    files.add(file);
  }

  @Override
  public void onProgressUpdate(int done, int total) {
    checkForCancel();
    this.done = done;
    this.total = total;
  }

  private static void checkForCancel() {
    if (Thread.interrupted()) {
      throw new CancellationException();
    }
  }

  final Cursor getSortedResult(@Nullable Comparator<VfsObject> comparator) {
    if (comparator == null) {
      return getSortedResult(DefaultComparator.instance());
    }
    Collections.sort(dirs, comparator);
    Collections.sort(files, comparator);
    return getResult();
  }

  final Cursor getResult() {
    final MatrixCursor cursor = new MatrixCursor(Schema.Listing.COLUMNS, dirs.size() + files.size());
    for (Schema.Listing.Dir d : schema.directories(dirs)) {
      cursor.addRow(d.serialize());
    }
    for (Schema.Listing.File f : schema.files(files)) {
      cursor.addRow(f.serialize());
    }
    return cursor;
  }

  final Cursor getStatus() {
    final MatrixCursor cursor = new MatrixCursor(Schema.Status.COLUMNS, 1);
    if (total != 0) {
      cursor.addRow(new Schema.Status.Progress(done, total).serialize());
    } else {
      cursor.addRow(Schema.Status.Progress.Companion.createIntermediate().serialize());
    }
    return cursor;
  }
}
