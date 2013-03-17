/*
 * @file
 * 
 * @brief Playlist database layer
 * 
 * @version $Id:$
 * 
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.playlist;

import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.text.TextUtils;
import android.util.Log;

/*
 * Playlist DB model.
 * 
 * version 1
 * 
 * Tables; 'playlist' - all entries
 *  fields: 
 *   '_id', integer primary autoincrement - unique entry identifier
 *   'URI', text - source data identifier including subpath etc 
 *   'TYPE', text - chiptune type identifier 
 *   'AUTHOR', text - cached/modified entry's author
 *   'TITLE', text - cached/modified entry's title
 *   'DURATION', integer - cached entry's duration in mS
 *   'TAGS', text - space separated list of hash prefixed words
 *   'PROPERTIES', text - comma separated list of name=value entries
 */

public class Database {

  private static final String NAME = "zxtune";
  private static final int VERSION = 1;
  private static final String TAG = "app.zxtune.playlist.Database";
  
  public static class Query {
    
    private String selection;
    private String orderBy;
    
    public Query(String selection, String orderBy) {
      this.selection = selection;
      this.orderBy = TextUtils.isEmpty(orderBy) ? Tables.Playlist.Fields._ID + " ASC" : orderBy;
    }
    
    public void setSelectionById(Long id) {
      if (id != null) {
        final String subselection = Tables.Playlist.Fields._ID + " = " + id;
        selection = TextUtils.isEmpty(selection) ? subselection : selection + " AND " + subselection;
      }
    }
    
    public String getSelection() {
      return selection;
    }
    
    public String getSortOrder() {
      return orderBy;
    }
  }

  public static final class Tables {
    public static final class Playlist {
      public static enum Fields {
        _ID,
        URI,
        TYPE,
        AUTHOR,
        TITLE,
        DURATION,
        TAGS,
        PROPERTIES
      }

      private static final String NAME = "playlist";
      private static final String CREATE_QUERY = "create table " + NAME + " (" + Fields._ID
          + " integer primary key autoincrement, " + Fields.URI + " text, " + Fields.TYPE + " text, "
          + Fields.AUTHOR + " text, " + Fields.TITLE + " text, " + Fields.DURATION + " integer, "
          + Fields.TAGS + " text, " + Fields.PROPERTIES + " text" + ");";
    }
  }

  private final DBHelper dbHelper;

  public Database(Context context) {
    this.dbHelper = new DBHelper(context);
  }

  //! @return Cursor with queried values
  public Cursor queryPlaylistItems(String[] columns, String selection, String[] selectionArgs, String orderBy) {
    Log.d(TAG, "queryPlaylistItems(" + selection + ") called");
    final SQLiteDatabase db = dbHelper.getReadableDatabase();
    return db.query(Tables.Playlist.NAME, columns, selection, selectionArgs, null/*groupBy*/, null/*having*/, orderBy);
  }
  
  //! @return new item id
  public long insertPlaylistItem(ContentValues values) {
    Log.d(TAG, "insertPlaylistItem() called");
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    return db.insert(Tables.Playlist.NAME, null, values);
  }
  
  //! @return count of deleted items
  public int deletePlaylistItems(String selection, String[] selectionArgs) {
    Log.d(TAG, "deletePlaylistItems(" + selection + ") called");
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    return db.delete(Tables.Playlist.NAME, selection, selectionArgs);
  }
  
  //! @return count of updated items
  public int updatePlaylistItems(ContentValues values, String selection, String[] selectionArgs) {
    Log.d(TAG, "updatePlaylistItems(" + selection + ") called");
    final SQLiteDatabase db = dbHelper.getWritableDatabase();
    return db.update(Tables.Playlist.NAME, values, selection, selectionArgs);
  }
  
  public static String defaultPlaylistOrder() {
    return Tables.Playlist.Fields._ID + " ASC";
  }
  
  private class DBHelper extends SQLiteOpenHelper {

    public DBHelper(Context context) {
      super(context, NAME, null, VERSION);
    }

    @Override
    public void onCreate(SQLiteDatabase db) {
      db.execSQL(Tables.Playlist.CREATE_QUERY);
    }

    @Override
    public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {}
  }
}
