package app.zxtune.fs.provider;

import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.Nullable;

import app.zxtune.fs.VfsObject;

class ParentsOperation implements AsyncQueryOperation {

  private final Uri uri;
  private final Resolver resolver;
  private final SchemaSource schema;

  ParentsOperation(Uri uri, Resolver resolver, SchemaSource schema) {
    this.uri = uri;
    this.resolver = resolver;
    this.schema = schema;
  }

  @Override
  public Cursor call() throws Exception {
    final VfsObject obj = resolver.resolve(uri);
    return ParentsCursorBuilder.makeParents(obj, schema);
  }

  @Nullable
  @Override
  public Cursor status() {
    return null;
  }
}
