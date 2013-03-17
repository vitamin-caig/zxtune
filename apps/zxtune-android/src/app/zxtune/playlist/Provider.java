/*
 * @file
 * 
 * @brief Playlist content provider
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
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
    final Query query = new Query(uri);
    final Database.Query dbQuery = new Database.Query(selection, sortOrder);
    dbQuery.setSelectionById(query.getId());
    final Cursor result = db.queryPlaylistItems(projection, dbQuery.getSelection(), selectionArgs, dbQuery.getSortOrder());
    result.setNotificationUri(getContext().getContentResolver(), Query.unparse(null));
    return result;
  }
  
  @Override
  public Uri insert(Uri uri, ContentValues values) {
    final Query query = new Query(uri);
    if (null != query.getId()) {
      throw new IllegalArgumentException("Wrong URI: " + uri); 
    }
    final long id = db.insertPlaylistItem(values);
    final Uri result = Query.unparse(id);
    getContext().getContentResolver().notifyChange(result, null);
    return result;
  }
  
  @Override
  public int delete(Uri uri, String selection, String[] selectionArgs) {
    final Query query = new Query(uri);
    final Database.Query dbQuery = new Database.Query(selection, null);
    dbQuery.setSelectionById(query.getId());
    final int count = db.deletePlaylistItems(dbQuery.getSelection(), selectionArgs);
    getContext().getContentResolver().notifyChange(uri, null);
    return count;
  }
  
  @Override
  public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
    final Query query = new Query(uri);
    final Database.Query dbQuery = new Database.Query(selection, null);
    dbQuery.setSelectionById(query.getId());
    final int count = db.updatePlaylistItems(values, dbQuery.getSelection(), selectionArgs);
    getContext().getContentResolver().notifyChange(uri, null);
    return count;
  }
  
  @Override
  public String getType(Uri uri) {
    try {
      final Query query = new Query(uri);
      return query.getMimeType();
    } catch (IllegalArgumentException e) {
      return null;
    }
  }
}
