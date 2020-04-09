package app.zxtune.fs.provider;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.NonNull;

// TODO: use provider notifications instead of polling
public class VfsProviderClient {

  public interface StatusCallback {
    void onProgress(int done, int total);
  }

  public interface ListingCallback extends StatusCallback {
    void onDir(Uri uri, String name, String description, int icon, boolean hasFeed);

    void onFile(Uri uri, String name, String description, String size);
  }

  public interface ParentsCallback extends StatusCallback {
    void onObject(Uri uri, String name, int icon);
  }

  private final ContentResolver resolver;

  public VfsProviderClient(Context ctx) {
    this.resolver = ctx.getContentResolver();
  }

  public final void resolve(@NonNull Uri uri, @NonNull ListingCallback cb) throws Exception {
    final Uri resolverUri = Query.resolveUriFor(uri);
    for (; ; ) {
      final Cursor cursor = resolver.query(resolverUri, null, null, null, null);
      if (cursor == null) {
        break;
      }
      try {
        if (!cursor.moveToFirst()) {
          break;
        } else if (Schema.Status.isStatus(cursor)) {
          getStatus(cursor, cb);
          waitOrCancel(resolverUri);
        } else {
          getObjects(cursor, cb);
          break;
        }
      } finally {
        cursor.close();
      }
    }
  }

  public final void list(@NonNull Uri uri, @NonNull ListingCallback cb) throws Exception {
    final Uri resolverUri = Query.listingUriFor(uri);
    for (; ; ) {
      final Cursor cursor = resolver.query(resolverUri, null, null, null, null);
      if (cursor == null) {
        break;
      }
      try {
        if (!cursor.moveToFirst()) {
          break;
        } else if (Schema.Status.isStatus(cursor)) {
          getStatus(cursor, cb);
          waitOrCancel(resolverUri);
        } else {
          getObjects(cursor, cb);
          break;
        }
      } finally {
        cursor.close();
      }
    }
  }

  public final void parents(@NonNull Uri uri, @NonNull ParentsCallback cb) throws Exception {
    final Uri resolverUri = Query.parentsUriFor(uri);
    for (; ; ) {
      final Cursor cursor = resolver.query(resolverUri, null, null, null, null);
      if (cursor == null) {
        break;
      }
      try {
        if (!cursor.moveToFirst()) {
          break;
        } else if (Schema.Status.isStatus(cursor)) {
          getStatus(cursor, cb);
          waitOrCancel(resolverUri);
        } else {
          getObjects(cursor, cb);
          break;
        }
      } finally {
        cursor.close();
      }
    }
  }

  public final void search(@NonNull Uri uri, @NonNull String query, @NonNull ListingCallback cb) throws Exception {
    final Uri resolverUri = Query.searchUriFor(uri, query);
    for (; ;) {
      checkForCancel(resolverUri);
      final Cursor cursor = resolver.query(resolverUri, null, null, null, null);
      if (cursor == null) {
        return;
      }
      try {
        if (cursor.moveToFirst()) {
          if (Schema.Status.isStatus(cursor)) {
            getStatus(cursor, cb);
          } else {
            getObjects(cursor, cb);
            if (cursor.isLast()) {
              break;
            }
          }
        } else {
          waitOrCancel(resolverUri);
        }
      } finally {
        cursor.close();
      }
    }
  }

  private void checkForCancel(@NonNull Uri resolverUri) throws Exception {
    if (Thread.interrupted()) {
      resolver.delete(resolverUri, null, null);
      throw new InterruptedException();
    }
  }

  private void waitOrCancel(@NonNull Uri resolverUri) throws Exception {
    try {
      Thread.sleep(1000);
    } catch (InterruptedException e) {
      resolver.delete(resolverUri, null, null);
      throw e;
    }
  }

  private static void getStatus(@NonNull Cursor cursor, @NonNull StatusCallback cb) throws Exception {
    final String err = Schema.Status.getError(cursor);
    if (err != null) {
      throw new Exception(err);
    } else {
      cb.onProgress(Schema.Status.getDone(cursor), Schema.Status.getTotal(cursor));
    }
  }

  private static void getObjects(@NonNull Cursor cursor, @NonNull ListingCallback cb) {
    do {
      if (Schema.Listing.isLimiter(cursor)) {
        break;
      }
      final Uri uri = Schema.Listing.getUri(cursor);
      final String name = Schema.Listing.getName(cursor);
      final String description = Schema.Listing.getDescription(cursor);
      if (Schema.Listing.isDir(cursor)) {
        cb.onDir(uri, name, description, Schema.Listing.getIcon(cursor),
            Schema.Listing.hasFeed(cursor));
      } else {
        cb.onFile(uri, name, description, Schema.Listing.getSize(cursor));
      }
    } while (cursor.moveToNext());
  }

  private static void getObjects(@NonNull Cursor cursor, @NonNull ParentsCallback cb) {
    do {
      final Uri uri = Schema.Parents.getUri(cursor);
      final String name = Schema.Parents.getName(cursor);
      final int icon = Schema.Parents.getIcon(cursor);
      cb.onObject(uri, name, icon);
    } while (cursor.moveToNext());
  }
}
