package app.zxtune.fs.provider;

import android.database.Cursor;
import android.database.MatrixCursor;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.concurrent.CancellationException;

import app.zxtune.fs.DefaultComparator;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.fs.VfsUtils;

class ListingCursorBuilder extends VfsDir.Visitor {

  private final ArrayList<VfsDir> dirs = new ArrayList<>();
  private final ArrayList<VfsFile> files = new ArrayList<>();
  private int total = -1;
  private int done = -1;

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
    for (VfsDir d : dirs) {
      cursor.addRow(makeRow(d));
    }
    for (VfsFile f : files) {
      cursor.addRow(makeRow(f));
    }
    return cursor;
  }

  private static Object[] makeRow(@NonNull VfsDir d) {
    return Schema.Listing.makeDirectory(d.getUri(), d.getName(),
        d.getDescription(), VfsUtils.getObjectIcon(d), hasFeed(d));
  }

  private static Object[] makeRow(@NonNull VfsFile f) {
    return Schema.Listing.makeFile(f.getUri(), f.getName(), f.getDescription(), f.getSize());
  }

  private static boolean hasFeed(@NonNull VfsObject obj) {
    return null != obj.getExtension(VfsExtensions.FEED);
  }

  final Cursor getStatus() {
    final MatrixCursor cursor = new MatrixCursor(Schema.Status.COLUMNS, 1);
    if (total != 0) {
      cursor.addRow(Schema.Status.makeProgress(done, total));
    } else {
      cursor.addRow(Schema.Status.makeIntermediateProgress());
    }
    return cursor;
  }

  //TODO: check out for separate schema
  static Cursor createSingleEntry(@NonNull VfsObject obj) {
    final MatrixCursor cursor = new MatrixCursor(Schema.Listing.COLUMNS, 1);
    if (obj instanceof VfsDir) {
      cursor.addRow(makeRow((VfsDir) obj));
    } else if (obj instanceof VfsFile) {
      cursor.addRow(makeRow((VfsFile) obj));
    }
    return cursor;
  }
}
