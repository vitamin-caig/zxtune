package app.zxtune.fs.provider;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;

import androidx.annotation.Nullable;
import androidx.core.os.OperationCanceledException;

import java.io.InterruptedIOException;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicReference;

import app.zxtune.Log;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsIterator;
import app.zxtune.fs.VfsObject;

class SearchOperation implements AsyncQueryOperation {

  private static final String TAG = SearchOperation.class.getName();

  private final Uri uri;
  private final Resolver resolver;
  private final SchemaSource schema;
  private final String query;
  private final AtomicReference<ArrayList<VfsFile>> result = new AtomicReference<>();

  SearchOperation(Uri uri, Resolver resolver, SchemaSource schema, String query) {
    this.uri = uri;
    this.resolver = resolver;
    this.schema = schema;
    this.query = query.toLowerCase();
  }

  @Override
  public Cursor call() throws Exception {
    final VfsDir dir = maybeResolve();
    result.set(new ArrayList<>());
    if (dir != null) {
      search(dir);
    }
    synchronized(result) {
      final MatrixCursor res = convert(result.getAndSet(null));
      res.addRow(Schema.Listing.Delimiter.INSTANCE.serialize());
      return res;
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

  private void search(VfsDir dir) throws Exception {
    final VfsExtensions.SearchEngine.Visitor visitor = obj -> {
      checkForCancel();
      synchronized(result) {
        result.get().add(obj);
      }
    };
    search(dir, visitor);
  }

  private void search(VfsDir dir, VfsExtensions.SearchEngine.Visitor visitor) throws Exception {
    final VfsExtensions.SearchEngine engine =
        (VfsExtensions.SearchEngine) dir.getExtension(VfsExtensions.SEARCH_ENGINE);
    if (engine != null) {
      engine.find(query, visitor);
    } else {
      final VfsIterator iter = new VfsIterator(dir, e -> {
        if (e instanceof InterruptedIOException) {
          throwCanceled();
        } else {
          Log.w(TAG, e, "Ignore I/O error");
        }
      });
      for (; iter.isValid(); iter.next()) {
        final VfsFile file = iter.getFile();
        if (match(file.getName()) || match(file.getDescription())) {
          visitor.onFile(file);
        } else {
          checkForCancel();
        }
      }
    }
  }

  private boolean match(String txt) {
    return txt.toLowerCase().contains(query);
  }

  private void checkForCancel() {
    if (Thread.interrupted()) {
      throwCanceled();
    }
  }

  private void throwCanceled() {
    Log.d(TAG, "Canceled search for %s at %s", query, uri);
    throw new OperationCanceledException();
  }

  @Nullable
  @Override
  public Cursor status() {
    synchronized(result) {
      return result.get() != null
          ? convert(result.getAndSet(new ArrayList<>()))
          : null;
    }
  }

  private MatrixCursor convert(ArrayList<VfsFile> found) {
    final MatrixCursor res = new MatrixCursor(Schema.Listing.COLUMNS, found.size() + 1);
    for (Schema.Listing.File f : schema.files(found)) {
      res.addRow(f.serialize());
    }
    return res;
  }
}
