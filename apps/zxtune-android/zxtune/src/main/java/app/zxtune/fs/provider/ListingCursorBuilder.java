package app.zxtune.fs.provider;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;

import androidx.annotation.Nullable;

import java.io.File;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.concurrent.CancellationException;

import app.zxtune.fs.DefaultComparator;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.fs.VfsUtils;

class ListingCursorBuilder extends VfsDir.Visitor {

  private final SchemaSource schema;
  private final ArrayList<VfsDir> dirs = new ArrayList<>();
  private final ArrayList<VfsFile> files = new ArrayList<>();
  private int total = -1;
  private int done = -1;

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

  private static Object[] makeRow(VfsDir d) {
    return Schema.Listing.makeDirectory(d.getUri(), d.getName(),
        d.getDescription(), VfsUtils.getObjectIcon(d), hasFeed(d));
  }

  private static Object[] makeRow(Uri uri, VfsFile f, @Nullable Integer tracks) {
    return Schema.Listing.makeFile(uri, f.getName(), f.getDescription(), f.getSize(), tracks,
        isCached(f));
  }

  private static boolean hasFeed(VfsObject obj) {
    return null != obj.getExtension(VfsExtensions.FEED);
  }

  @Nullable
  private static Boolean isCached(VfsFile obj) {
    final File f = Vfs.getCache(obj);
    return f != null ? f.isFile() : null;
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

  //TODO: check out for separate schema
  static Cursor createSingleEntry(VfsObject obj) {
    final MatrixCursor cursor = new MatrixCursor(Schema.Listing.COLUMNS, 1);
    if (obj instanceof VfsDir) {
      cursor.addRow(makeRow((VfsDir) obj));
    } else if (obj instanceof VfsFile) {
      final Uri uri = obj.getUri();
      // don't care about real tracks count here
      cursor.addRow(makeRow(uri, (VfsFile) obj, null));
    }
    return cursor;
  }
}
