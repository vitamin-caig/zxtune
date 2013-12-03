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
import android.content.ContentValues;
import android.database.Cursor;
import android.net.Uri;

public class Provider extends ContentProvider {

  private Database db;

  @Override
  public boolean onCreate() {
    db = new Database(getContext());
    return true;
  }

  @Override
  public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
    final Long id = PlaylistQuery.idOf(uri);
    final String select = id != null ? PlaylistQuery.selectionFor(id) : selection;
    final Cursor result = db.queryPlaylistItems(projection, select, selectionArgs, sortOrder);
    result.setNotificationUri(getContext().getContentResolver(), PlaylistQuery.ALL);
    return result;
  }
  
  @Override
  public Uri insert(Uri uri, ContentValues values) {
    final Long id = PlaylistQuery.idOf(uri);
    if (null != id) {
      throw new IllegalArgumentException("Wrong URI: " + uri); 
    }
    final long result = db.insertPlaylistItem(values);
    //do not notify about change
    return PlaylistQuery.uriFor(result);
  }
  
  @Override
  public int delete(Uri uri, String selection, String[] selectionArgs) {
    final Long id = PlaylistQuery.idOf(uri);
    final int count = id != null 
      ? db.deletePlaylistItems(PlaylistQuery.selectionFor(id), null)
      : db.deletePlaylistItems(selection, selectionArgs);
    getContext().getContentResolver().notifyChange(uri, null);
    return count;
  }
  
  @Override
  public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
    final Long id = PlaylistQuery.idOf(uri);
    final int count = id != null
      ? db.updatePlaylistItems(values, PlaylistQuery.selectionFor(id), null)
      : db.updatePlaylistItems(values, selection, selectionArgs);
    getContext().getContentResolver().notifyChange(uri, null);
    return count;
  }
  
  @Override
  public String getType(Uri uri) {
    try {
      return PlaylistQuery.mimeTypeOf(uri);
    } catch (IllegalArgumentException e) {
      return null;
    }
  }
}
