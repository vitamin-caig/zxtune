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

/*
 * content://app.zxtune.playlist/items - all items
 * content://app.zxtune.playlist/items/$id - item with id
 */

public class Query {
  
  private static final class QueryType {
    public final String mimeType;
    public final int uriType;
    
    QueryType(String mime) {
      mimeType = mime;
      uriType = Math.abs(mimeType.hashCode());
    }
  }
  
  private static final class Playlist {
    public static final String AUTHORITY = "app.zxtune.playlist";
    public static final String ITEMS_PATH = "items";
    
    public static final QueryType MULTIPLE_ITEMS;
    public static final QueryType SINGLE_ITEM;
    
    static {
      MULTIPLE_ITEMS = new QueryType("vnd.android.cursor.dir/vnd." + AUTHORITY + "." + ITEMS_PATH);
      SINGLE_ITEM = new QueryType("vnd.android.cursor.item/vnd." + AUTHORITY + "." + ITEMS_PATH);
    }
  }
  
  private static final UriMatcher playlistUriTemplate;
  
  static {
    playlistUriTemplate = new UriMatcher(UriMatcher.NO_MATCH);
    playlistUriTemplate.addURI(Playlist.AUTHORITY, Playlist.ITEMS_PATH, Playlist.MULTIPLE_ITEMS.uriType);
    playlistUriTemplate.addURI(Playlist.AUTHORITY, Playlist.ITEMS_PATH + "/#", Playlist.SINGLE_ITEM.uriType);
  }
  
  private final QueryType type;
  private final Long id;
  
  public Query(Uri uri) {
    final int uriType = playlistUriTemplate.match(uri);
    if (uriType == Playlist.MULTIPLE_ITEMS.uriType) {
      type = Playlist.MULTIPLE_ITEMS;
      id = null;
    } else if (uriType == Playlist.SINGLE_ITEM.uriType) {
      type = Playlist.SINGLE_ITEM;
      id = ContentUris.parseId(uri);
    } else {
      throw new IllegalArgumentException("Wrong URI: " + uri);
    }
  }
  
  public Long getId() {
    return id;
  }
  
  public String getMimeType() {
    return type.mimeType;
  }
  
  public static Uri unparse(Long id) {
    final Uri.Builder builder = new Uri.Builder()
      .scheme(ContentResolver.SCHEME_CONTENT)
      .authority(Playlist.AUTHORITY)
      .path(Playlist.ITEMS_PATH);
    if (id != null) {
      builder.appendPath(id.toString());
    }
    return builder.build();
  }
}
