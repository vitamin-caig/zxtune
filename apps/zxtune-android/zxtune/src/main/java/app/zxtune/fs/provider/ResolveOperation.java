package app.zxtune.fs.provider;

import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.NonNull;
import androidx.collection.LruCache;

import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsObject;

class ResolveOperation implements AsyncQueryOperation {

  private final Uri uri;
  private final LruCache<Uri, VfsObject> cache;
  private VfsObject result;

  ResolveOperation(@NonNull Uri uri, LruCache<Uri, VfsObject> cache) {
    this.uri = uri;
    this.cache = cache;
    this.result = cache.get(uri);
  }

  @Override
  public Cursor call() throws Exception {
    maybeResolve();
    if (result != null) {
      cache.put(uri, result);
      return ListingCursorBuilder.createSingleEntry(result);
    } else {
      return null;
    }
  }

  private void maybeResolve() throws Exception {
    if (result == null) {
      result = VfsArchive.resolveForced(uri);
    }
  }

  @Override
  public Cursor status() {
    return StatusBuilder.makeProgress(-1);
  }
}
