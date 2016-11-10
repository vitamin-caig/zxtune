/**
 * 
 * @file
 * 
 * @brief Playlist query helper
 * 
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.playlist;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.UriMatcher;
import android.net.Uri;

import java.util.Arrays;

import app.zxtune.playlist.Database.Tables;

/*
 * content://app.zxtune.playlist/items - all items
 * content://app.zxtune.playlist/items/$id - single item with specific id
 */

public class PlaylistQuery {

  private static final class Type {

    public final String mime;
    public final int id;

    Type(String mime) {
      this.mime = mime;
      this.id = Math.abs(mime.hashCode());
    }
  }

  private static final String AUTHORITY = "app.zxtune.playlist";
  private static final String ITEMS_PATH = "items";
  private static final String STATISTICS_PATH = "statistics";

  private static final Type TYPE_ALL_ITEMS;
  private static final Type TYPE_ONE_ITEM;
  private static final Type TYPE_STATISTICS;

  private static final UriMatcher uriTemplate;

  public static final Uri ALL;
  public static final Uri STATISTICS;

  static {
    TYPE_ALL_ITEMS = new Type("vnd.android.cursor.dir/vnd." + AUTHORITY + "." + ITEMS_PATH);
    TYPE_ONE_ITEM = new Type("vnd.android.cursor.item/vnd." + AUTHORITY + "." + ITEMS_PATH);
    TYPE_STATISTICS = new Type("vnd.android.cursor.item/vnd." + AUTHORITY + "." + STATISTICS_PATH);
    uriTemplate = new UriMatcher(UriMatcher.NO_MATCH);
    uriTemplate.addURI(AUTHORITY, ITEMS_PATH, TYPE_ALL_ITEMS.id);
    uriTemplate.addURI(AUTHORITY, ITEMS_PATH + "/#", TYPE_ONE_ITEM.id);
    uriTemplate.addURI(AUTHORITY, STATISTICS_PATH, TYPE_STATISTICS.id);
    ALL = uriFor(null);
    STATISTICS = new Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT).authority(AUTHORITY).path(STATISTICS_PATH).build();
  }

  //! @return Mime type of uri (used in content provider)
  static String mimeTypeOf(Uri uri) {
    final int uriType = uriTemplate.match(uri);
    if (uriType == TYPE_ALL_ITEMS.id) {
      return TYPE_ALL_ITEMS.mime;
    } else if (uriType == TYPE_ONE_ITEM.id) {
      return TYPE_ONE_ITEM.mime;
    } else if (uriType == TYPE_STATISTICS.id) {
      return TYPE_STATISTICS.mime;
    } else {
      throw new IllegalArgumentException("Wrong URI: " + uri);
    }
  }

  public static Long idOf(Uri uri) {
    final int uriType = uriTemplate.match(uri);
    if (uriType == TYPE_ALL_ITEMS.id) {
      return null;
    } else if (uriType == TYPE_ONE_ITEM.id) {
      return ContentUris.parseId(uri);
    } else {
      throw new IllegalArgumentException("Wrong URI: " + uri);
    }
  }

  public static Uri uriFor(Long id) {
    final Uri.Builder builder =
        new Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT).authority(AUTHORITY)
            .path(ITEMS_PATH);
    if (id != null) {
      builder.appendPath(id.toString());
    }
    return builder.build();
  }
  
  public static String selectionFor(long id) {
    return Database.Tables.Playlist.Fields._id + " = " + id;
  }

  public static String selectionFor(long ids[]) {
    if (ids == null) {
      return null;
    } else {
      //ids => '[a, b, c]'
      final String rawArgs = Arrays.toString(ids);
      final String args = rawArgs.substring(1, rawArgs.length() - 1);
      //placeholders doesn't work and has limitations
      return Database.Tables.Playlist.Fields._id + " IN (" + args + ")";
    }
  }

  public static String limitedOrder(int count) {
    return count > 0
        ? Tables.Playlist.Fields.pos + " ASC LIMIT " + count
        : Tables.Playlist.Fields.pos + " DESC LIMIT " + (-count);
  }

  public static String positionSelection(String comparing, Long id) {
    return String.format("%1$s %2$s (SELECT %1$s from %3$s WHERE %4$s=%5$d)",
        Tables.Playlist.Fields.pos,
        comparing,
        Tables.Playlist.NAME,
        Tables.Playlist.Fields._id,
        id);
  }
}
