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
      final Long curId = new Query(item.getUri()).getId();
      final String selection = Database.Tables.Playlist.Fields._id + " > ?";
      next = advance(curId, selection, Database.Tables.Playlist.Fields._id + " ASC LIMIT 1");
    }
    return new DatabaseIterator(resolver, next);
  }
  
  public final DatabaseIterator getPrev() {
    Item prev = null;
    if (isValid()) {
      final Long curId = new Query(item.getUri()).getId();
      final String selection = Database.Tables.Playlist.Fields._id + " < ?";
      prev = advance(curId, selection, Database.Tables.Playlist.Fields._id + " DESC LIMIT 1");
    }
    return new DatabaseIterator(resolver, prev);
  }
  
  public final DatabaseIterator getFirst() {
    Item first = null;
    if (isValid()) {
      first = select(Database.Tables.Playlist.Fields._id + " ASC LIMIT 1");
    }
    return new DatabaseIterator(resolver, first);
  }
  
  public final DatabaseIterator getLast() {
    Item last = null;
    if (isValid()) {
      last = select(Database.Tables.Playlist.Fields._id + " DESC LIMIT 1");
    }
    return new DatabaseIterator(resolver, last);
  }
  
  public final DatabaseIterator getRandom() {
    final Item rand = select("RANDOM() LIMIT 1");
    return new DatabaseIterator(resolver, rand);
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
