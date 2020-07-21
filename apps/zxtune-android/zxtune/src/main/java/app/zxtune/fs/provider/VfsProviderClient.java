package app.zxtune.fs.provider;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;

import androidx.annotation.Nullable;

// TODO: use provider notifications instead of polling
public class VfsProviderClient {

  interface StatusCallback {
    void onProgress(int done, int total);
  }

  public interface ListingCallback extends StatusCallback {
    void onDir(Uri uri, String name, String description, @Nullable Integer icon, boolean hasFeed);

    void onFile(Uri uri, String name, String description, String details, @Nullable Integer tracks,
                @Nullable Boolean cached);
  }

  public interface ParentsCallback extends StatusCallback {
    void onObject(Uri uri, String name, @Nullable Integer icon);
  }

  private final ContentResolver resolver;

  public VfsProviderClient(Context ctx) {
    this.resolver = ctx.getContentResolver();
  }

  public final void resolve(Uri uri, ListingCallback cb) throws Exception {
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

  public final void list(Uri uri, ListingCallback cb) throws Exception {
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

  public final void parents(Uri uri, ParentsCallback cb) throws Exception {
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

  public final void search(Uri uri, String query, ListingCallback cb) throws Exception {
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

  public static Uri getFileUriFor(Uri uri) {
    return Query.fileUriFor(uri);
  }

  private void checkForCancel(Uri resolverUri) throws Exception {
    if (Thread.interrupted()) {
      resolver.delete(resolverUri, null, null);
      throw new InterruptedException();
    }
  }

  private void waitOrCancel(Uri resolverUri) throws Exception {
    try {
      Thread.sleep(1000);
    } catch (InterruptedException e) {
      resolver.delete(resolverUri, null, null);
      throw e;
    }
  }

  private static void getStatus(Cursor cursor, StatusCallback cb) throws Exception {
    final String err = Schema.Status.getError(cursor);
    if (err != null) {
      throw new Exception(err);
    } else {
      cb.onProgress(Schema.Status.getDone(cursor), Schema.Status.getTotal(cursor));
    }
  }

  private static void getObjects(Cursor cursor, ListingCallback cb) {
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
        cb.onFile(uri, name, description, Schema.Listing.getDetails(cursor),
            Schema.Listing.getTracks(cursor), Schema.Listing.isCached(cursor));
      }
    } while (cursor.moveToNext());
  }

  private static void getObjects(Cursor cursor, ParentsCallback cb) {
    do {
      final Uri uri = Schema.Parents.getUri(cursor);
      final String name = Schema.Parents.getName(cursor);
      final Integer icon = Schema.Parents.getIcon(cursor);
      cb.onObject(uri, name, icon);
    } while (cursor.moveToNext());
  }
}
