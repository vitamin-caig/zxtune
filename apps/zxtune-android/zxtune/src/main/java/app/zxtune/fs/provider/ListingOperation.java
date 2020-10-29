package app.zxtune.fs.provider;

import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.Comparator;

import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsObject;

class ListingOperation implements AsyncQueryOperation {

  private final ListingCursorBuilder builder =
      new ListingCursorBuilder(VfsArchive::getModulesCount);
  private final Uri uri;
  @Nullable
  private VfsDir dir;

  ListingOperation(Uri uri) {
    this.uri = uri;
  }

  ListingOperation(VfsDir dir) {
    this.uri = dir.getUri();
    this.dir = dir;
  }

  @SuppressWarnings("unchecked")
  @Nullable
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
