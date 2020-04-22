package app.zxtune.fs.provider;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.io.File;
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

  interface TracksCountSource {
    @NonNull
    Integer[] getTracksCount(@NonNull Uri[] uris);
  }

  private final TracksCountSource tracksCountSource;
  private final ArrayList<VfsDir> dirs = new ArrayList<>();
  private final ArrayList<VfsFile> files = new ArrayList<>();
  private int total = -1;
  private int done = -1;

  ListingCursorBuilder(TracksCountSource src) {
    this.tracksCountSource = src;
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
    for (VfsDir d : dirs) {
      cursor.addRow(makeRow(d));
    }
    final Uri[] uris = getFilesUris();
    final Integer[] tracks = tracksCountSource.getTracksCount(uris);
    for (int i = 0; i < uris.length; ++i) {
      final VfsFile file = files.get(i);
      cursor.addRow(makeRow(uris[i], file, tracks[i]));
    }
    return cursor;
  }

  private Uri[] getFilesUris() {
    final Uri[] result = new Uri[files.size()];
    for (int i = 0; i < result.length; ++i) {
      result[i] = files.get(i).getUri();
    }
    return result;
  }

  private static Object[] makeRow(@NonNull VfsDir d) {
    return Schema.Listing.makeDirectory(d.getUri(), d.getName(),
        d.getDescription(), VfsUtils.getObjectIcon(d), hasFeed(d));
  }

  private static Object[] makeRow(@NonNull Uri uri, @NonNull VfsFile f, Integer tracks) {
    return Schema.Listing.makeFile(uri, f.getName(), f.getDescription(), f.getSize(), tracks,
        isCached(f));
  }

  private static boolean hasFeed(@NonNull VfsObject obj) {
    return null != obj.getExtension(VfsExtensions.FEED);
  }

  private static Boolean isCached(@NonNull VfsObject obj) {
    final File f = (File) obj.getExtension(VfsExtensions.CACHE);
    return f != null ? f.isFile() : null;
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
      final Uri uri = obj.getUri();
      // don't care about real tracks count here
      cursor.addRow(makeRow(uri, (VfsFile) obj, null));
    }
    return cursor;
  }
}
