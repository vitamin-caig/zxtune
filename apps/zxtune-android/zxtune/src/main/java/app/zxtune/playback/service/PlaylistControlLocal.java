/**
 * @file
 * @brief Local ContentProvider-based implementation of PlaylistControl
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.playback.service;

import android.content.Context;
import android.database.Cursor;
import androidx.annotation.Nullable;
import android.util.SparseIntArray;
import app.zxtune.analytics.Analytics;
import app.zxtune.playback.PlaylistControl;
import app.zxtune.playlist.Database;
import app.zxtune.playlist.PlaylistQuery;

class PlaylistControlLocal implements PlaylistControl {

  private final Context context;
  private final Database db;

  public PlaylistControlLocal(Context context) {
    this.context = context;
    this.db = new Database(context);
  }

  @Override
  public void delete(long[] ids) {
    deleteItems(PlaylistQuery.selectionFor(ids));
    Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_DELETE, ids.length);
  }

  @Override
  public void deleteAll() {
    deleteItems(null);
    Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_DELETE, 0);
  }

  private void deleteItems(@Nullable String selection) {
    db.deletePlaylistItems(selection, null);
    notifyPlaylist();
  }

  private void notifyPlaylist() {
    context.getContentResolver().notifyChange(PlaylistQuery.ALL, null);
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
  @Override
  public void move(long id, int delta) {
    final SparseIntArray positions = getNewPositions(id, delta);
    db.updatePlaylistItemsOrder(positions);
    notifyPlaylist();
    Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_MOVE, 1);
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

  @Override
  public void sort(SortBy by, SortOrder order) {
    final Database.Tables.Playlist.Fields field = Database.Tables.Playlist.Fields.valueOf(by.name());
    final String orderString = order == SortOrder.desc ? "DESC" : "ASC";
    db.sortPlaylistItems(field, orderString);
    notifyPlaylist();
    Analytics.sendPlaylistEvent(Analytics.PLAYLIST_ACTION_SORT,
        100 * by.ordinal() + order.ordinal());
  }
}
