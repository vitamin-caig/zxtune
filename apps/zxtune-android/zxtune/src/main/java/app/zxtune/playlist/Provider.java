/**
 *
 * @file
 *
 * @brief Playlist content provider component
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playlist;

import android.content.ContentProvider;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.support.annotation.NonNull;

public class Provider extends ContentProvider {

  private Database db;
  private ContentResolver resolver;

  @Override
  public boolean onCreate() {
    final Context ctx = getContext();
    if (ctx != null) {
      db = new Database(ctx);
      resolver = ctx.getContentResolver();
      return true;
    } else {
      return false;
    }
  }

  @Override
  public Cursor query(@NonNull Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
    if (PlaylistQuery.STATISTICS.equals(uri)) {
      return db.queryStatistics(selection);
    } else {
      final Long id = PlaylistQuery.idOf(uri);
      final String select = id != null ? PlaylistQuery.selectionFor(id) : selection;
      final Cursor result = db.queryPlaylistItems(projection, select, selectionArgs, sortOrder);
      result.setNotificationUri(resolver, PlaylistQuery.ALL);
      return result;
    }
  }
  
  @Override
  public Uri insert(@NonNull Uri uri, ContentValues values) {
    final Long id = PlaylistQuery.idOf(uri);
    if (null != id) {
      throw new IllegalArgumentException("Wrong URI: " + uri); 
    }
    final long result = db.insertPlaylistItem(values);
    //do not notify about change
    return PlaylistQuery.uriFor(result);
  }
  
  @Override
  public int delete(@NonNull Uri uri, String selection, String[] selectionArgs) {
    final Long id = PlaylistQuery.idOf(uri);
    final int count = id != null 
      ? db.deletePlaylistItems(PlaylistQuery.selectionFor(id), null)
      : db.deletePlaylistItems(selection, selectionArgs);
    resolver.notifyChange(uri, null);
    return count;
  }
  
  @Override
  public int update(@NonNull Uri uri, ContentValues values, String selection, String[] selectionArgs) {
    final Long id = PlaylistQuery.idOf(uri);
    final int count = id != null
      ? db.updatePlaylistItems(values, PlaylistQuery.selectionFor(id), null)
      : db.updatePlaylistItems(values, selection, selectionArgs);
    resolver.notifyChange(uri, null);
    return count;
  }
  
  @Override
  public String getType(@NonNull Uri uri) {
    try {
      return PlaylistQuery.mimeTypeOf(uri);
    } catch (IllegalArgumentException e) {
      return null;
    }
  }
}
