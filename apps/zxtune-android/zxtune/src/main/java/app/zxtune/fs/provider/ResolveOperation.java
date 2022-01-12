package app.zxtune.fs.provider;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;

import androidx.annotation.Nullable;
import androidx.core.os.OperationCanceledException;

import app.zxtune.Log;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;

class ResolveOperation implements AsyncQueryOperation {

  private static final String TAG = ResolveOperation.class.getName();

  private final Uri uri;
  private final Resolver resolver;
  private final SchemaSource schema;
  private final int[] progress = {-1, 100};

  ResolveOperation(Uri uri, Resolver resolver, SchemaSource schema) {
    this.uri = uri;
    this.resolver = resolver;
    this.schema = schema;
  }

  @Nullable
  @Override
  public Cursor call() throws Exception {
    final VfsObject result = maybeResolve();
    if (result != null) {
      return makeResult(result);
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

  private Cursor makeResult(VfsObject vfsObj) {
    final MatrixCursor cursor = new MatrixCursor(Schema.Listing.COLUMNS, 1);
    final Schema.Object obj = schema.resolved(vfsObj);
    if (obj != null) {
      cursor.addRow(obj.serialize());
    }
    return cursor;
  }
}
