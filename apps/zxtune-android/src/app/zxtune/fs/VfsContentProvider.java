/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;
import android.util.Log;

public class VfsContentProvider extends ContentProvider {
  
  private final static String TAG = VfsContentProvider.class.getName();
  
  private VfsRoot root;
  
  @Override
  public boolean onCreate() {
    return true;
  }

  @Override
  public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
    final VfsQuery query = new VfsQuery(uri);
    final Uri path = query.getPath();
    Log.d(TAG, String.format("query(%s) = %s", uri.toString(), path.toString()));
    final VfsDir dir = (VfsDir) getRoot().resolve(path);
    final Cursor result = new VfsCursor(dir);
    result.setNotificationUri(getContext().getContentResolver(), uri);
    return result;
  }

  @Override
  public String getType(Uri uri) {
    try {
      final VfsQuery query = new VfsQuery(uri);
      return query.getMimeType();
    } catch (IllegalArgumentException e) {
      return null;
    }
  }

  @Override
  public int delete(Uri uri, String selection, String[] selectionArgs) {
    return 0;
  }

  @Override
  public Uri insert(Uri uri, ContentValues values) {
    return null;
  }

  @Override
  public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
    return 0;
  }
  
  private VfsRoot getRoot() {
    if (root == null) {
      root = Vfs.createRoot(getContext());
    }
    return root;
  }
}
