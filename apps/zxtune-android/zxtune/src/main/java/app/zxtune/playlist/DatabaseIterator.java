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

import androidx.annotation.Nullable;

import java.util.Random;

public class DatabaseIterator {

  private final static Random rand = new Random();
  
  private final ContentResolver resolver;
  @Nullable
  private final Item item;

  public DatabaseIterator(Context context, Uri current) {
    this.resolver = context.getContentResolver();
    this.item = loadItem(current);
  }
  
  private DatabaseIterator(ContentResolver resolver, @Nullable Item current) {
    this.resolver = resolver;
    this.item = current;
  }

  @Nullable
  public final Item getItem() {
    return item;
  }
  
  public final boolean isValid() {
    return item != null;
  }
  
  public final DatabaseIterator getNext() {
    Item next = null;
    if (item != null) {
      final Long curId = PlaylistQuery.idOf(item.getUri());
      if (curId != null) {
        final String selection = PlaylistQuery.positionSelection(">", curId);
        next = selectFirstFrom(selection);
      }
    }
    return new DatabaseIterator(resolver, next);
  }
  
  public final DatabaseIterator getPrev() {
    Item prev = null;
    if (item != null) {
      final Long curId = PlaylistQuery.idOf(item.getUri());
      if (curId != null) {
        final String selection = PlaylistQuery.positionSelection("<", curId);
        prev = selectLastFrom(selection);
      }
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
    final Cursor cursor = resolver.query(PlaylistQuery.ALL, null, null, null, null);
    if (cursor != null) {
      try {
        if (cursor.moveToFirst() && cursor.move(rand.nextInt(cursor.getCount()))) {
          return new DatabaseIterator(resolver, new Item(cursor));
        }
      } finally {
        cursor.close();
      }
    }
    return new DatabaseIterator(resolver, null);
  }

  @Nullable
  private Item selectFirstFrom(@Nullable String selection) {
    return select(selection, PlaylistQuery.limitedOrder(1));
  }

  @Nullable
  private Item selectLastFrom(@Nullable String selection) {
    return select(selection, PlaylistQuery.limitedOrder(-1));
  }

  @Nullable
  private Item select(@Nullable String selection, String order) {
    final Cursor cursor = resolver.query(PlaylistQuery.ALL, null, selection, null, order);
    return loadItem(cursor);
  }

  @Nullable
  private Item loadItem(Uri id) {
    final Cursor cursor = resolver.query(id, null, null, null, null);
    return loadItem(cursor);
  }

  @Nullable
  private Item loadItem(@Nullable Cursor cursor) {
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
