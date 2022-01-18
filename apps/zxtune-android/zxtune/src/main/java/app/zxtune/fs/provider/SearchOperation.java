package app.zxtune.fs.provider;

import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;

import androidx.annotation.Nullable;
import androidx.core.os.OperationCanceledException;

import java.io.InterruptedIOException;
import java.util.concurrent.atomic.AtomicReference;

import app.zxtune.Log;
import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsIterator;
import app.zxtune.fs.VfsObject;

class SearchOperation implements AsyncQueryOperation {

  private static final String TAG = SearchOperation.class.getName();

  private final Uri uri;
  private final Resolver resolver;
  private final String query;
  private final AtomicReference<ListingCursorBuilder> result = new AtomicReference<>();

  SearchOperation(Uri uri, Resolver resolver, String query) {
    this.uri = uri;
    this.resolver = resolver;
    this.query = query;
  }

  @Override
  public Cursor call() throws Exception {
    final VfsDir dir = maybeResolve();
    if (dir != null) {
      result.set(makeBuilder());
      search(dir);
    }
    synchronized(result) {
      final MatrixCursor res = (MatrixCursor) result.getAndSet(null).getResult();
      res.addRow(Schema.Listing.makeLimiter());
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
        result.get().onFile(obj);
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
          ? result.getAndSet(makeBuilder()).getResult()
          : null;
    }
  }

  private static ListingCursorBuilder makeBuilder() {
    return new ListingCursorBuilder(VfsArchive::getModulesCount);
  }
}
