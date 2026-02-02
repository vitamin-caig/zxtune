/**
 * @file
 * @brief Playlist query helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.playlist;

import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.UriMatcher;
import android.net.Uri;

import androidx.annotation.Nullable;

import app.zxtune.BuildConfig;

/*
 * content://app.zxtune.playlist/items - all items
 * content://app.zxtune.playlist/items/$id - single item with specific id
 */

public class PlaylistQuery {

  private static final class Type {

    final String mime;
    final int id;

    private Type(String base, String path) {
      this.mime = base + "/vnd." + AUTHORITY + "." + path;
      this.id = Math.abs(mime.hashCode());
    }

    static Type dir(String path) {
      return new Type(ContentResolver.CURSOR_DIR_BASE_TYPE, path);
    }

    static Type item(String path) {
      return new Type(ContentResolver.CURSOR_ITEM_BASE_TYPE, path);
    }
  }

  private static final String AUTHORITY = BuildConfig.APPLICATION_ID + ".playlist";
  private static final String ITEMS_PATH = "items";
  private static final String SAVED_PATH = "saved";

  private static final Type TYPE_ALL_ITEMS;
  private static final Type TYPE_ONE_ITEM;
  private static final Type TYPE_SAVED;

  private static final UriMatcher uriTemplate;

  public static final Uri ALL;
  public static final Uri SAVED;

  static {
    TYPE_ALL_ITEMS = Type.dir(ITEMS_PATH);
    TYPE_ONE_ITEM = Type.item(ITEMS_PATH);
    TYPE_SAVED = Type.dir(SAVED_PATH);
    uriTemplate = new UriMatcher(UriMatcher.NO_MATCH);
    uriTemplate.addURI(AUTHORITY, ITEMS_PATH, TYPE_ALL_ITEMS.id);
    uriTemplate.addURI(AUTHORITY, ITEMS_PATH + "/#", TYPE_ONE_ITEM.id);
    uriTemplate.addURI(AUTHORITY, SAVED_PATH, TYPE_SAVED.id);

    ALL = uriFor(null);
    SAVED = uriForPath(SAVED_PATH).build();
  }

  private static Uri.Builder uriForPath(String path) {
    return new Uri.Builder().scheme(ContentResolver.SCHEME_CONTENT).authority(AUTHORITY)
        .path(path);
  }

  public static boolean isPlaylistUri(Uri uri) {
    final int type = uriTemplate.match(uri);
    return type != -1;
  }

  //! @return Mime type of uri (used in content provider)
  static String mimeTypeOf(Uri uri) {
    final int uriType = uriTemplate.match(uri);
    if (uriType == TYPE_ALL_ITEMS.id) {
      return TYPE_ALL_ITEMS.mime;
    } else if (uriType == TYPE_ONE_ITEM.id) {
      return TYPE_ONE_ITEM.mime;
    } else if (uriType == TYPE_SAVED.id) {
      return TYPE_SAVED.mime;
    } else {
      throw new IllegalArgumentException("Wrong URI: " + uri);
    }
  }

  @Nullable
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

  public static Uri uriFor(@Nullable Long id) {
    final Uri.Builder builder = uriForPath(ITEMS_PATH);
    if (id != null) {
      builder.appendPath(id.toString());
    }
    return builder.build();
  }
}
