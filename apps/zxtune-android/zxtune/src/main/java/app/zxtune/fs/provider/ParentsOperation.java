package app.zxtune.fs.provider;

import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.Nullable;

import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsObject;

class ParentsOperation implements AsyncQueryOperation {

  private final Uri uri;
  @Nullable
  private VfsObject obj;

  ParentsOperation(Uri uri) {
    this.uri = uri;
  }

  ParentsOperation(VfsObject obj) {
    this.uri = obj.getUri();
    this.obj = obj;
  }

  @Override
  public Cursor call() throws Exception {
    maybeResolve();
    return ParentsCursorBuilder.makeParents(obj);
  }

  private void maybeResolve() throws Exception {
    if (obj == null) {
      obj = VfsArchive.resolve(uri);
    }
  }

  @Nullable
  @Override
  public Cursor status() {
    return null;
  }
}
