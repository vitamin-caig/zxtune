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
  private final Item item;

  public Iterator(Context context, Uri current) {
    this.resolver = context.getContentResolver();
    this.item = loadItem(current);
  }
  
  private Iterator(ContentResolver resolver, Item current) {
    this.resolver = resolver;
    this.item = current;
  }

  public Item getItem() {
    return item;
  }
  
  public boolean isValid() {
    return item != null;
  }
  
  public Iterator getNext() {
    Item next = null;
    if (isValid()) {
      final Long curId = new Query(item.getUri()).getId();
      final String selection = Database.Tables.Playlist.Fields._id + " > ?";
      next = advance(curId, selection, Database.Tables.Playlist.Fields._id + " ASC LIMIT 1");
    }
    return new Iterator(resolver, next);
  }
  
  public Iterator getPrev() {
    Item prev = null;
    if (isValid()) {
      final Long curId = new Query(item.getUri()).getId();
      final String selection = Database.Tables.Playlist.Fields._id + " < ?";
      prev = advance(curId, selection, Database.Tables.Playlist.Fields._id + " DESC LIMIT 1");
    }
    return new Iterator(resolver, prev);
  }
  
  public Iterator getFirst() {
    Item first = null;
    if (isValid()) {
      first = select(Database.Tables.Playlist.Fields._id + " ASC LIMIT 1");
    }
    return new Iterator(resolver, first);
  }
  
  public Iterator getLast() {
    Item last = null;
    if (isValid()) {
      last = select(Database.Tables.Playlist.Fields._id + " DESC LIMIT 1");
    }
    return new Iterator(resolver, last);
  }

  private Item advance(Long curId, String selection, String order) {
    final Cursor cursor = resolver.query(Query.unparse(null), null, selection, new String[] {curId.toString()}, order);
    return loadItem(cursor);
  }
  
  private Item select(String order) {
    final String selection = "*";
    final Cursor cursor = resolver.query(Query.unparse(null), null, selection, null, order);
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
