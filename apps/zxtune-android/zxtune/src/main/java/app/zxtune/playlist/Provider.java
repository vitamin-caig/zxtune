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
import android.database.CursorWrapper;
import android.net.Uri;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.util.Arrays;

public class Provider extends ContentProvider {

  private Database db;
  private ContentResolver resolver;
  private final ActiveRowState activeRow;

  public Provider() {
    this.activeRow = new ActiveRowState();
  }

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
      final Cursor dbCursor = db.queryPlaylistItems(projection, select, selectionArgs, sortOrder);
      final Cursor result = projection == null ? new MergedStatusCursor(dbCursor, activeRow) : dbCursor;
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
    if (id != null) {
      final int activeRowsUpdated = activeRow.update(id, values);
      if (activeRowsUpdated != 0) {
        resolver.notifyChange(PlaylistQuery.ALL, null);
        return activeRowsUpdated;
      } else {
        final int result = db.updatePlaylistItems(values, PlaylistQuery.selectionFor(id), null);
        resolver.notifyChange(uri, null);
        return result;
      }
    } else {
      final int result = db.updatePlaylistItems(values, selection, selectionArgs);
      resolver.notifyChange(PlaylistQuery.ALL, null);
      return result;
    }
  }

  @Override
  public String getType(@NonNull Uri uri) {
    try {
      return PlaylistQuery.mimeTypeOf(uri);
    } catch (IllegalArgumentException e) {
      return null;
    }
  }

  private static class ActiveRowState {
    private long id = -1;
    private Object[] values = new Object[Fields.values().length];

    //should be suffix of Database.Tables.Playlist.Fields
    //TODO: generalize
    private static enum Fields {
      state
    }

    synchronized int update(long id, ContentValues data) {
      final Object[] newValues = parse(data);
      if (newValues != null) {
        final int result = this.id != id ? 2 : 1;
        this.id = id;
        this.values = newValues;
        return result;
      } else {
        return 0;
      }
    }

    @Nullable
    private static Object[] parse(ContentValues data) {
      Object[] result = null;
      for (Fields f : Fields.values()) {
        if (data.containsKey(f.name())) {
          if (result == null) {
            result = new Object[Fields.values().length];
          }
          result[f.ordinal()] = data.get(f.name());
        }
      }
      return result;
    }

    synchronized int getInt(Cursor src, int idx) {
      return isThisRow(src) ? getInt(idx) : 0;
    }

    private boolean isThisRow(Cursor src) {
      return id == src.getLong(Database.Tables.Playlist.Fields._id.ordinal());
    }

    private int getInt(int idx) {
      final Object val = values[idx];
      return val instanceof Integer
              ? ((Integer) val)
              : 0;
    }
  }

  private static class MergedStatusCursor extends CursorWrapper {

    private final ActiveRowState state;
    private final int dbColumns;

    public MergedStatusCursor(Cursor db, ActiveRowState state) {
      super(db);
      this.state = state;
      this.dbColumns = super.getColumnCount();
    }

    @Override
    public int getColumnCount() {
      return super.getColumnCount() + ActiveRowState.Fields.values().length;
    }

    @Override
    public int getColumnIndex(String columnName) {
      for (ActiveRowState.Fields f : ActiveRowState.Fields.values()) {
        if (f.name().equals(columnName)) {
          return f.ordinal();
        }
      }
      return super.getColumnIndexOrThrow(columnName);
    }

    @Override
    public int getColumnIndexOrThrow(String columnName) {
      for (ActiveRowState.Fields f : ActiveRowState.Fields.values()) {
        if (f.name().equals(columnName)) {
          return f.ordinal();
        }
      }
      return super.getColumnIndexOrThrow(columnName);
    }

    @Override
    public String getColumnName(int index) {
      throw new UnsupportedOperationException("getColumnName");
    }

    @Override
    public String[] getColumnNames() {
      final String[] existing = super.getColumnNames();
      final ActiveRowState.Fields[] extra = ActiveRowState.Fields.values();
      final String[] result = Arrays.copyOf(existing, existing.length + extra.length);
      for (int i = 0; i < extra.length; ++i) {
        result[existing.length + i] = extra[i].name();
      }
      return result;
    }

    @Override
    public int getInt(int index) {
      if (index < dbColumns) {
        return super.getInt(index);
      } else {
        return state.getInt(getWrappedCursor(), index - dbColumns);
      }
    }
  }
}
