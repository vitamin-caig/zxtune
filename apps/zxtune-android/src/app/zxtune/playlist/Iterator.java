/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.playlist;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;

/**
 * 
 */
public class Iterator {
  
  private final ContentResolver resolver;
  private Item item;

  public Iterator(Context context) {
    this.resolver = context.getContentResolver();
    this.item = null;
  }
  
  public Iterator(Context context, Uri current) {
    this.resolver = context.getContentResolver();
    this.item = loadItem(current);
  }

  public Item getCurrentItem() {
    return item;
  }
  
  public boolean isValid() {
    return item != null;
  }
  
  public void next() {
    if (isValid()) {
      final Long curId = new Query(item.getUri()).getId();
      final String selection = Database.Tables.Playlist.Fields._id + " > ?";
      item = advance(curId, selection, Database.Tables.Playlist.Fields._id + " ASC LIMIT 1");
    }
  }
  
  public void prev() {
    if (isValid()) {
      final Long curId = new Query(item.getUri()).getId();
      final String selection = Database.Tables.Playlist.Fields._id + " < ?";
      item = advance(curId, selection, Database.Tables.Playlist.Fields._id + " DESC LIMIT 1");
    }
  }

  private Item advance(Long curId, String selection, String order) {
    final Cursor cursor = resolver.query(Query.unparse(null), null, selection, new String[] {curId.toString()}, order);
    return loadItem(cursor);
  }
  
  private Item loadItem(Uri id) {
    final Cursor cursor = resolver.query(id, null, null, null, null);
    return loadItem(cursor);
  }
  
  private final Item loadItem(Cursor cursor) {
    try {
      return cursor != null && cursor.moveToFirst()
        ? new Item(cursor)
        : null;
    } finally {
      if (cursor != null) {
        cursor.close();
      }
    }
  }
}
