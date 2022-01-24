package app.zxtune.fs.provider;

import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.Comparator;

import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsObject;

class ListingOperation implements AsyncQueryOperation {

  private final Uri uri;
  private final Resolver resolver;
  private final ListingCursorBuilder builder;

  ListingOperation(Uri uri, Resolver resolver, SchemaSource schema) {
    this.uri = uri;
    this.resolver = resolver;
    this.builder = new ListingCursorBuilder(schema);
  }

  @SuppressWarnings("unchecked")
  @Nullable
  @Override
  public Cursor call() throws Exception {
    final VfsDir dir = maybeResolve();
    if (dir != null) {
      dir.enumerate(builder);
      return builder.getSortedResult((Comparator<VfsObject>) dir.getExtension(VfsExtensions.COMPARATOR));
    } else {
      return null;
    }
  }

  @Nullable
  private VfsDir maybeResolve() throws Exception {
    final VfsObject obj = resolver.resolve(uri);
    if (obj instanceof VfsDir) {
      return (VfsDir) obj;
    }
    return null;
  }

  @Override
  public Cursor status() {
    return builder.getStatus();
  }
}
