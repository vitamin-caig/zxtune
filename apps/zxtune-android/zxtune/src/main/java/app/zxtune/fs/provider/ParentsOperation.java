package app.zxtune.fs.provider;

import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.Nullable;

import app.zxtune.fs.VfsObject;

class ParentsOperation implements AsyncQueryOperation {

  private final Uri uri;
  private final Resolver resolver;

  ParentsOperation(Uri uri, Resolver resolver) {
    this.uri = uri;
    this.resolver = resolver;
  }

  @Override
  public Cursor call() throws Exception {
    final VfsObject obj = resolver.resolve(uri);
    return ParentsCursorBuilder.makeParents(obj);
  }

  @Nullable
  @Override
  public Cursor status() {
    return null;
  }
}
