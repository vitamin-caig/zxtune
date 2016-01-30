/**
 *
 * @file
 *
 * @brief Database iterator helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.playlist;

import android.content.ContentResolver;
import android.content.Context;
import android.database.Cursor;
import android.net.Uri;

public class DatabaseIterator {
  
  private final ContentResolver resolver;
  private final Item item;

  public DatabaseIterator(Context context, Uri current) {
    this.resolver = context.getContentResolver();
    this.item = loadItem(current);
  }
  
  private DatabaseIterator(ContentResolver resolver, Item current) {
    this.resolver = resolver;
    this.item = current;
  }

  public final Item getItem() {
    return item;
  }
  
  public final boolean isValid() {
    return item != null;
  }
  
  public final DatabaseIterator getNext() {
    Item next = null;
    if (isValid()) {
      final Long curId = PlaylistQuery.idOf(item.getUri());
      final String selection = PlaylistQuery.positionSelection(">", curId); 
      next = selectFirstFrom(selection);
    }
    return new DatabaseIterator(resolver, next);
  }
  
  public final DatabaseIterator getPrev() {
    Item prev = null;
    if (isValid()) {
      final Long curId = PlaylistQuery.idOf(item.getUri());
      final String selection = PlaylistQuery.positionSelection("<", curId);
      prev = selectLastFrom(selection);
    }
    return new DatabaseIterator(resolver, prev);
  }
  
  public final DatabaseIterator getFirst() {
    Item first = null;
    if (isValid()) {
      first = selectFirstFrom(null);
    }
    return new DatabaseIterator(resolver, first);
  }
  
  public final DatabaseIterator getLast() {
    Item last = null;
    if (isValid()) {
      last = selectLastFrom(null);
    }
    return new DatabaseIterator(resolver, last);
  }
  
  public final DatabaseIterator getRandom() {
    final Item rand = select("RANDOM() LIMIT 1");
    return new DatabaseIterator(resolver, rand);
  }
  
  private Item selectFirstFrom(String selection) {
    return select(selection, PlaylistQuery.limitedOrder(1));
  }
  
  private Item selectLastFrom(String selection) {
    return select(selection, PlaylistQuery.limitedOrder(-1));
  }

  private Item select(String order) {
    return select(null, order);
  }

  private Item select(String selection, String order) {
    final Cursor cursor = resolver.query(PlaylistQuery.ALL, null, selection, null, order);
    return loadItem(cursor);
  }
  
  private Item loadItem(Uri id) {
    final Cursor cursor = resolver.query(id, null, null, null, null);
    return loadItem(cursor);
  }
  
  private Item loadItem(Cursor cursor) {
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
