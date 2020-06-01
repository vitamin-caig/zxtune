/**
 * @file
 * @brief Playlist content provider component
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.playlist;

import android.content.ContentProvider;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.SparseIntArray;

import androidx.annotation.Nullable;

import app.zxtune.MainApplication;

public class Provider extends ContentProvider {

  private static final String METHOD_SORT = "sort";
  private static final String METHOD_MOVE = "move";

  @Nullable
  private Database db;
  @Nullable
  private ContentResolver resolver;

  @Override
  public boolean onCreate() {
    final Context ctx = getContext();
    if (ctx != null) {
      MainApplication.initialize(ctx.getApplicationContext());
      db = new Database(ctx);
      resolver = ctx.getContentResolver();
      return true;
    } else {
      return false;
    }
  }

  @Nullable
  @Override
  public Cursor query(Uri uri, @Nullable String[] projection, @Nullable String selection,
                      @Nullable String[] selectionArgs, @Nullable String sortOrder) {
    if (PlaylistQuery.STATISTICS.equals(uri)) {
      return db.queryStatistics(selection);
    } else {
      final Long id = PlaylistQuery.idOf(uri);
      final String select = id != null ? PlaylistQuery.selectionFor(id) : selection;
      final Cursor dbCursor = db.queryPlaylistItems(projection, select, selectionArgs, sortOrder);
      dbCursor.setNotificationUri(resolver, PlaylistQuery.ALL);
      return dbCursor;
    }
  }

  @Nullable
  @Override
  public Uri insert(Uri uri, @Nullable ContentValues values) {
    final Long id = PlaylistQuery.idOf(uri);
    if (null != id) {
      throw new IllegalArgumentException("Wrong URI: " + uri);
    }
    final long result = db.insertPlaylistItem(values);
    //do not notify about change
    return PlaylistQuery.uriFor(result);
  }

  @Override
  public int delete(Uri uri, @Nullable String selection, @Nullable String[] selectionArgs) {
    final Long id = PlaylistQuery.idOf(uri);
    return id != null
               ? db.deletePlaylistItems(PlaylistQuery.selectionFor(id), null)
               : db.deletePlaylistItems(selection, selectionArgs);
  }

  @Override
  public int update(Uri uri, @Nullable ContentValues values, @Nullable String selection,
                    @Nullable String[] selectionArgs) {
    final Long id = PlaylistQuery.idOf(uri);
    if (id != null) {
      return db.updatePlaylistItems(values, PlaylistQuery.selectionFor(id), null);
    } else {
      return db.updatePlaylistItems(values, selection, selectionArgs);
    }
  }

  static void sort(ContentResolver resolver, String by, String order) {
    resolver.call(PlaylistQuery.ALL, METHOD_SORT, by + " " + order, null);
  }

  static void move(ContentResolver resolver, long id, int delta) {
    resolver.call(PlaylistQuery.ALL, METHOD_MOVE,
        id + " " + delta, null);
  }

  @Nullable
  @Override
  public Bundle call(String method, @Nullable String arg, @Nullable Bundle extras) {
    if (arg == null) {
      return null;
    }
    final String[] args = TextUtils.split(arg, " ");
    if (METHOD_SORT.equals(method)) {
      sort(args[0], args[1]);
    } else if (METHOD_MOVE.equals(method)) {
      move(Long.parseLong(args[0]), Integer.parseInt(args[1]));
    }
    return null;
  }

  private void sort(String fieldName, String order) {
    final Database.Tables.Playlist.Fields field =
        Database.Tables.Playlist.Fields.valueOf(fieldName);
    db.sortPlaylistItems(field, order);
  }

  /*
   * @param id item to move
   * @param delta position change
   */

  /*
   * pos idx     pos idx
   *       <-+
   * p0  i0  |   p0  i0 -+
   * p1  i1  |   p1  i1  |
   * p2  i2  |   p2  i2  |
   * p3  i3  |   p3  i3  |
   * p4  i4  |   p4  i4  |
   * p5  i5 -+   p5  i5  |
   *                   <-+
   *
   * move(i5,-5) move(i0,5)
   *
   * select:
   * i5 i4 i3 i2 i1 i0
   * p5 p4 p3 p2 p1 p0
   *
   *             i0 i1 i2 i3 i4 i5
   *             p0 p1 p2 p3 p4 p5
   *
   * p0  i5      p0  i1
   * p1  i0      p1  i2
   * p2  i1      p2  i3
   * p3  i2      p3  i4
   * p4  i3      p4  i5
   * p5  i4      p5  i0
   *
   */
  private void move(long id, int delta) {
    final SparseIntArray positions = getNewPositions(id, delta);
    db.updatePlaylistItemsOrder(positions);
  }

  private SparseIntArray getNewPositions(long id, int delta) {
    final String[] proj = {Database.Tables.Playlist.Fields._id.name(), Database.Tables.Playlist.Fields.pos.name()};
    final String sel = PlaylistQuery.positionSelection(delta > 0 ? ">=" : "<=", id);
    final int count = Math.abs(delta) + 1;
    final String ord = PlaylistQuery.limitedOrder(delta > 0 ? delta + 1 : delta - 1);
    final Cursor cur = db.queryPlaylistItems(proj, sel, null, ord);
    final int[] ids = new int[count];
    final int[] pos = new int[count];
    try {
      int i = 0;
      while (cur.moveToNext()) {
        ids[(i + count - 1) % count] = cur.getInt(0);
        pos[i] = cur.getInt(1);
        ++i;
      }
    } finally {
      cur.close();
    }
    final SparseIntArray res = new SparseIntArray();
    for (int i = 0; i != count; ++i) {
      res.append(ids[i], pos[i]);
    }
    return res;
  }

  @Nullable
  @Override
  public String getType(Uri uri) {
    try {
      return PlaylistQuery.mimeTypeOf(uri);
    } catch (IllegalArgumentException e) {
      return null;
    }
  }
}
