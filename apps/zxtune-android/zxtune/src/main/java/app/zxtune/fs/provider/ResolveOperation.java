package app.zxtune.fs.provider;

import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.Nullable;
import androidx.collection.LruCache;
import androidx.core.os.OperationCanceledException;

import app.zxtune.Log;
import app.zxtune.fs.ProgressCallback;
import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsObject;

class ResolveOperation implements AsyncQueryOperation {

  private static final String TAG = ResolveOperation.class.getName();

  private final Uri uri;
  private final LruCache<Uri, VfsObject> cache;
  @Nullable
  private VfsObject result;
  private final int[] progress = {-1, -1};

  ResolveOperation(Uri uri, LruCache<Uri, VfsObject> cache) {
    this.uri = uri;
    this.cache = cache;
    this.result = cache.get(uri);
  }

  @Nullable
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
      result = VfsArchive.resolveForced(uri, (done, total) -> {
        checkForCancel();
        progress[0] = done;
        progress[1] = total;
      });
    }
  }

  private void checkForCancel() {
    if (Thread.interrupted()) {
      Log.d(TAG, "Canceled resolving for %s", uri);
      throw new OperationCanceledException();
    }
  }


  @Override
  public Cursor status() {
    return StatusBuilder.makeProgress(progress[0], progress[1]);
  }
}
