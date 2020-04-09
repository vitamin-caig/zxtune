package app.zxtune.fs.provider;

import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.NonNull;

import java.util.Comparator;

import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsObject;

class ListingOperation implements AsyncQueryOperation {

  final ListingCursorBuilder builder = new ListingCursorBuilder();
  private final Uri uri;
  private VfsDir dir;

  ListingOperation(@NonNull Uri uri) {
    this.uri = uri;
  }

  ListingOperation(@NonNull VfsDir dir) {
    this.uri = dir.getUri();
    this.dir = dir;
  }

  @SuppressWarnings("unchecked")
  @Override
  public Cursor call() throws Exception {
    maybeResolve();
    if (dir != null) {
      dir.enumerate(builder);
      return builder.getSortedResult((Comparator<VfsObject>) dir.getExtension(VfsExtensions.COMPARATOR));
    } else {
      return null;
    }
  }

  private void maybeResolve() throws Exception {
    if (dir == null) {
      final VfsObject obj = VfsArchive.resolve(uri);
      if (obj instanceof VfsDir) {
        dir = (VfsDir) obj;
      }
    }
  }

  @Override
  public Cursor status() {
    return builder.getStatus();
  }
}
