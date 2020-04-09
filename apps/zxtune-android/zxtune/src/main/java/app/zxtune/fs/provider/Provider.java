package app.zxtune.fs.provider;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Handler;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.collection.LruCache;

import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.FutureTask;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;

import app.zxtune.Log;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsObject;

public class Provider extends ContentProvider {

  private static final String TAG = Provider.class.getName();

  private static final int CACHE_SIZE = 10;

  private final ExecutorService executor = Executors.newCachedThreadPool();
  private final ConcurrentHashMap<Uri, OperationHolder> operations = new ConcurrentHashMap<>();
  private final LruCache<Uri, VfsObject> objectsCache = new LruCache<Uri, VfsObject>(CACHE_SIZE) {
    @Override
    protected void entryRemoved(boolean evicted, @NonNull Uri key, @NonNull VfsObject oldValue,
                                @Nullable VfsObject newValue) {
      if (evicted) {
        Log.d(TAG, "Remove cache for " + key);
      }
    }
  };
  private final Handler handler = new Handler();

  @Override
  public boolean onCreate() {
    return true;
  }

  @Nullable
  @Override
  public Cursor query(@NonNull Uri uri, @Nullable String[] projection, @Nullable String selection, @Nullable String[] selectionArgs, @Nullable String sortOrder) {
    try {
      final OperationHolder existing = operations.get(uri);
      if (existing != null) {
        return existing.status();
      } else {
        final AsyncQueryOperation op = createOperation(uri);
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
  private AsyncQueryOperation createOperation(@NonNull Uri uri) {
    final Uri path = Query.getPathFrom(uri);
    switch (Query.getUriType(uri)) {
      case Query.TYPE_RESOLVE:
        return new ResolveOperation(path, objectsCache);
      case Query.TYPE_LISTING:
        return createListingOperation(path, objectsCache.get(uri));
      case Query.TYPE_PARENTS:
        return createParentsOperation(path, objectsCache.get(uri));
      case Query.TYPE_SEARCH:
        return createSearchOperation(path, objectsCache.get(uri), Query.getQueryFrom(uri));
      default:
        throw new UnsupportedOperationException("Unsupported uri " + uri);
    }
  }

  @Nullable
  private AsyncQueryOperation createListingOperation(@NonNull Uri uri, @Nullable VfsObject cached) {
    if (cached instanceof VfsDir) {
      return new ListingOperation((VfsDir) cached);
    } else if (cached == null) {
      return new ListingOperation(uri);
    } else {
      return null;
    }
  }

  private AsyncQueryOperation createParentsOperation(@NonNull Uri uri, @Nullable VfsObject cached) {
    if (cached != null) {
      return new ParentsOperation(cached);
    } else {
      return new ParentsOperation(uri);
    }
  }

  @Nullable
  private AsyncQueryOperation createSearchOperation(@NonNull Uri uri, @Nullable VfsObject cached,
                                                    @NonNull String query) {
    if (cached instanceof VfsDir) {
      return new SearchOperation((VfsDir) cached, query);
    } else if (cached == null) {
      return new SearchOperation(uri, query);
    } else {
      return null;
    }
  }

  @Nullable
  @Override
  public String getType(@NonNull Uri uri) {
    return Query.mimeTypeOf(uri);
  }

  @Nullable
  @Override
  public Uri insert(@NonNull Uri uri, @Nullable ContentValues values) {
    return null;
  }

  @Override
  public int delete(@NonNull Uri uri, @Nullable String selection, @Nullable String[] selectionArgs) {
    final OperationHolder op = operations.remove(uri);
    if (op != null) {
      op.cancel();
      return 1;
    } else {
      return 0;
    }
  }

  @Override
  public int update(@NonNull Uri uri, @Nullable ContentValues values, @Nullable String selection, @Nullable String[] selectionArgs) {
    return 0;
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

    OperationHolder(Uri uri, @NonNull AsyncQueryOperation op) {
      this.uri = uri;
      this.op = op;
      this.task = new FutureTask<>(op);
      this.update = new Runnable() {
        @Override
        public void run() {
          notifyUpdate(OperationHolder.this.uri);
          scheduleUpdate();
        }
      };
    }

    final Cursor start() throws Exception {
      executor.execute(task);
      try {
        return task.get(1, TimeUnit.SECONDS);
      } catch (TimeoutException e) {
        schedule();
        return op.status();
      }
    }

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
