package app.zxtune.fs.provider;

import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.Nullable;
import androidx.core.os.OperationCanceledException;

import app.zxtune.Log;
import app.zxtune.fs.VfsObject;

class ResolveOperation implements AsyncQueryOperation {

  private static final String TAG = ResolveOperation.class.getName();

  private final Uri uri;
  private final Resolver resolver;
  private final int[] progress = {-1, -1};

  ResolveOperation(Uri uri, Resolver resolver) {
    this.uri = uri;
    this.resolver = resolver;
  }

  @Nullable
  @Override
  public Cursor call() throws Exception {
    final VfsObject result = maybeResolve();
    if (result != null) {
      return ListingCursorBuilder.createSingleEntry(result);
    } else {
      return null;
    }
  }

  @Nullable
  private VfsObject maybeResolve() throws Exception {
    return resolver.resolve(uri, (done, total) -> {
      checkForCancel();
      progress[0] = done;
      progress[1] = total;
    });
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
