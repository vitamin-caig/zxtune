/**
 * 
 * @file
 * 
 * @brief Playlist query helper
 * 
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs.archives;

import android.content.ContentResolver;
import android.content.UriMatcher;
import android.net.Uri;

/*
 * ${path} is full data uri (including subpath in fragment) stored as string
 * 
 * content://app.zxtune.archive/archive/${path} - get archive info by specified full path
 * content://app.zxtune.archive/info/${path} - get single element with specified subpath 
 * content://app.zxtune.archive/dir/${path} - list all elements by specified subpath without recursion
 */

public class Query {

  private static final class Type {

    public final String mime;
    public final int id;

    Type(String mime) {
      this.mime = mime;
      this.id = Math.abs(mime.hashCode());
    }
  }

  private static final String AUTHORITY = "app.zxtune.archive";
  private static final String ARCHIVE_PATH = "archive";
  private static final String INFO_PATH = "info";
  private static final String DIR_PATH = "dir";
  
  private static final Type TYPE_ARCHIVE_ITEM;
  private static final Type TYPE_INFO_ITEM;
  private static final Type TYPE_DIR_ITEMS;

  private static final UriMatcher uriTemplate;

  static {
    TYPE_ARCHIVE_ITEM = new Type("vnd.android.cursor.item/vnd." + AUTHORITY + "." + ARCHIVE_PATH);
    TYPE_INFO_ITEM = new Type("vnd.android.cursor.item/vnd." + AUTHORITY + "." + INFO_PATH);
    TYPE_DIR_ITEMS = new Type("vnd.android.cursor.dir/vnd." + AUTHORITY + "." + DIR_PATH);
    uriTemplate = new UriMatcher(UriMatcher.NO_MATCH);
    uriTemplate.addURI(AUTHORITY, ARCHIVE_PATH + "/*", TYPE_ARCHIVE_ITEM.id);
    uriTemplate.addURI(AUTHORITY, INFO_PATH + "/*", TYPE_INFO_ITEM.id);
    uriTemplate.addURI(AUTHORITY, DIR_PATH + "/*", TYPE_DIR_ITEMS.id);
  }

  //! @return Mime type of uri (used in content provider)
  static String mimeTypeOf(Uri uri) {
    final int uriType = uriTemplate.match(uri);
    if (uriType == TYPE_ARCHIVE_ITEM.id) {
      return TYPE_ARCHIVE_ITEM.mime;
    } else if (uriType == TYPE_INFO_ITEM.id) {
      return TYPE_INFO_ITEM.mime;
    } else if (uriType == TYPE_DIR_ITEMS.id) {
      return TYPE_DIR_ITEMS.mime;
    } else {
      throw new IllegalArgumentException("Wrong URI: " + uri);
    }
  }

  public static Uri getPathFrom(Uri uri) {
    final int uriType = uriTemplate.match(uri);
    if (uriType == TYPE_ARCHIVE_ITEM.id) {
      return Uri.parse(uri.getLastPathSegment());
    } else if (uriType == TYPE_INFO_ITEM.id) {
      return Uri.parse(uri.getLastPathSegment());
    } else if (uriType == TYPE_DIR_ITEMS.id) {
      return Uri.parse(uri.getLastPathSegment());
    } else {
      throw new IllegalArgumentException("Wrong URI: " + uri);
    }
  }
  
  public static Uri archiveUriFor(Uri uri) {
      final Uri.Builder builder = new Uri.Builder()
      .scheme(ContentResolver.SCHEME_CONTENT)
      .authority(AUTHORITY)
      .path(ARCHIVE_PATH)
      .appendPath(uri.toString());
    return builder.build();
  }

  public static boolean isArchiveUri(Uri uri) {
    final int uriType = uriTemplate.match(uri);
    return uriType == TYPE_ARCHIVE_ITEM.id;
  }
  
  public static Uri infoUriFor(Uri uri) {
    final Uri.Builder builder = new Uri.Builder()
      .scheme(ContentResolver.SCHEME_CONTENT)
      .authority(AUTHORITY)
      .path(INFO_PATH)
      .appendPath(uri.toString());
    return builder.build();
  }

  public static boolean isInfoUri(Uri uri) {
    final int uriType = uriTemplate.match(uri);
    return uriType == TYPE_INFO_ITEM.id;
  }

  public static Uri listDirUriFor(Uri uri) {
    final Uri.Builder builder = new Uri.Builder()
      .scheme(ContentResolver.SCHEME_CONTENT)
      .authority(AUTHORITY)
      .path(DIR_PATH)
      .appendPath(uri.toString());
    return builder.build();
  }
  
  public static boolean isListDirUri(Uri uri) {
    final int uriType = uriTemplate.match(uri);
    return uriType == TYPE_DIR_ITEMS.id;
  }
}
