package app.zxtune.fs.provider;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Handler;
import android.os.ParcelFileDescriptor;

import androidx.annotation.Nullable;

import java.io.FileNotFoundException;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import app.zxtune.Log;
import app.zxtune.MainApplication;

public class Provider extends ContentProvider {

  private static final String TAG = Provider.class.getName();

  private static final int CACHE_SIZE = 10;

  private final ExecutorService executor = Executors.newCachedThreadPool();
  private final ConcurrentHashMap<Uri, OperationHolder> operations = new ConcurrentHashMap<>();
  private final Resolver resolver = new CachingResolver(CACHE_SIZE);
  private final SchemaSource schema = new SchemaSourceImplementation();
  private final Handler handler = new Handler();

  @Override
  public boolean onCreate() {
    final Context ctx = getContext();
    if (ctx != null) {
      MainApplication.initialize(ctx.getApplicationContext());
      return true;
    } else {
      return false;
    }
  }

  @Nullable
  @Override
  public Cursor query(Uri uri, @Nullable String[] projection, @Nullable String selection, @Nullable String[] selectionArgs, @Nullable String sortOrder) {
    try {
      final OperationHolder existing = operations.get(uri);
      if (existing != null) {
        return existing.status();
      } else {
        final AsyncQueryOperation op = createOperation(uri, projection);
        if (op == null) {
          return null;
        }
        final OperationHolder newOne = new OperationHolder(uri, op);
        return newOne.start();
      }
    } catch (Exception e) {
      return StatusBuilder.makeError(e);
    }
  }

  @Nullable
  private AsyncQueryOperation createOperation(Uri uri, @Nullable String[] projection) {
    final Uri path = Query.getPathFrom(uri);
    switch (Query.getUriType(uri)) {
      case Query.TYPE_RESOLVE:
        return new ResolveOperation(path, resolver, schema);
      case Query.TYPE_LISTING:
        return new ListingOperation(path, resolver, schema);
      case Query.TYPE_PARENTS:
        return new ParentsOperation(path, resolver, schema);
      case Query.TYPE_SEARCH:
        return new SearchOperation(path, resolver, schema, Query.getQueryFrom(uri));
      case Query.TYPE_FILE:
        return new FileOperation(path, resolver, projection);
      default:
        throw new UnsupportedOperationException("Unsupported uri " + uri);
    }
  }

  @Nullable
  @Override
  public String getType(Uri uri) {
    return Query.mimeTypeOf(uri);
  }

  @Nullable
  @Override
  public Uri insert(Uri uri, @Nullable ContentValues values) {
    return null;
  }

  @Override
  public int delete(Uri uri, @Nullable String selection, @Nullable String[] selectionArgs) {
    final OperationHolder op = operations.remove(uri);
    if (op != null) {
      op.cancel();
      return 1;
    } else {
      return 0;
    }
  }

  @Override
  public int update(Uri uri, @Nullable ContentValues values, @Nullable String selection, @Nullable String[] selectionArgs) {
    return 0;
  }

  @Override
  public ParcelFileDescriptor openFile(Uri uri, String mode)
      throws FileNotFoundException {
    try {
      final AsyncQueryOperation op = createOperation(uri, null);
      if (op instanceof FileOperation) {
        return ((FileOperation) op).openFile(mode);
      }
    } catch (Exception e) {
      Log.w(TAG, e, "Failed to open file " + uri);
    }
    throw new FileNotFoundException(uri.toString());
  }

  private void notifyUpdate(Uri uri) {
    final Context ctx = getContext();
    if (ctx != null) {
      ctx.getContentResolver().notifyChange(uri, null);
    }
  }

  private class OperationHolder {
    private final Uri uri;
    private final AsyncQueryOperation op;
    private final FutureTask<Cursor> task;
    private final Runnable update;

    OperationHolder(Uri uri, AsyncQueryOperation op) {
      this.uri = uri;
      this.op = op;
      this.task = new FutureTask<>(op);
      this.update = () -> {
        notifyUpdate(OperationHolder.this.uri);
        scheduleUpdate();
      };
    }

    @Nullable
    final Cursor start() throws Exception {
      executor.execute(task);
      try {
        return task.get(1, TimeUnit.SECONDS);
      } catch (TimeoutException e) {
        schedule();
        return op.status();
      }
    }

    @Nullable
    final Cursor status() throws Exception {
      if (task.isDone()) {
        unschedule();
        notifyUpdate(uri);
        return task.get();
      } else {
        return op.status();
      }
    }

    final void cancel() {
      task.cancel(true);
      unschedule();
    }

    private void schedule() {
      operations.put(uri, this);
      scheduleUpdate();
    }

    private void unschedule() {
      handler.removeCallbacks(update);
      operations.remove(uri);
    }

    private void scheduleUpdate() {
      handler.postDelayed(update, 1000);
    }
  }
}
