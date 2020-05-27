/**
 * @file
 * @brief Playlist query helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.provider;

import android.content.ContentResolver;
import android.content.UriMatcher;
import android.net.Uri;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;
import java.util.List;

import static java.lang.annotation.RetentionPolicy.SOURCE;

/*
 * ${path} is full data uri (including subpath in fragment) stored as string
 *
 * content://app.zxtune.vfs/resolve/${path} - get object properties by full path
 * content://app.zxtune.vfs/listing/${path} - get directory content by full path
 * content://app.zxtune.vfs/parents/${path} - get object parents chain
 * content://app.zxtune.vfs/search/${path}?query=${query} - start search
 * content://app.zxtune.vfs/file/${path} - get information about and file object about local file
 */

class Query {

  @Retention(SOURCE)
  @IntDef({TYPE_RESOLVE, TYPE_LISTING, TYPE_PARENTS, TYPE_SEARCH, TYPE_FILE})
  @interface Type {}

  static final int TYPE_RESOLVE = 0;
  static final int TYPE_LISTING = 1;
  static final int TYPE_PARENTS = 2;
  static final int TYPE_SEARCH = 3;
  static final int TYPE_FILE = 4;

  private static final String AUTHORITY = "app.zxtune.vfs";
  private static final String RESOLVE_PATH = "resolve";
  private static final String LISTING_PATH = "listing";
  private static final String PARENTS_PATH = "parents";
  private static final String SEARCH_PATH = "search";
  private static final String QUERY_PARAM = "query";
  private static final String FILE_PATH = "file";

  private static final String ITEM_SUBTYPE = "vnd." + AUTHORITY + ".item";
  private static final String SIMPLE_ITEM_SUBTYPE = "vnd." + AUTHORITY + ".simple_item";

  private static final String MIME_ITEM =
      ContentResolver.CURSOR_ITEM_BASE_TYPE + "/" + ITEM_SUBTYPE;
  private static final String MIME_ITEMS_SET =
      ContentResolver.CURSOR_DIR_BASE_TYPE + "/" + ITEM_SUBTYPE;
  private static final String MIME_SIMPLE_ITEMS_SET =
      ContentResolver.CURSOR_DIR_BASE_TYPE + "/" + SIMPLE_ITEM_SUBTYPE;

  private static final UriMatcher uriTemplate;

  static {
    uriTemplate = new UriMatcher(UriMatcher.NO_MATCH);
    uriTemplate.addURI(AUTHORITY, RESOLVE_PATH, TYPE_RESOLVE);
    uriTemplate.addURI(AUTHORITY, RESOLVE_PATH + "/*", TYPE_RESOLVE);
    uriTemplate.addURI(AUTHORITY, LISTING_PATH, TYPE_LISTING);
    uriTemplate.addURI(AUTHORITY, LISTING_PATH + "/*", TYPE_LISTING);
    uriTemplate.addURI(AUTHORITY, PARENTS_PATH, TYPE_PARENTS);
    uriTemplate.addURI(AUTHORITY, PARENTS_PATH + "/*", TYPE_PARENTS);
    uriTemplate.addURI(AUTHORITY, SEARCH_PATH + "*", TYPE_SEARCH);
    uriTemplate.addURI(AUTHORITY, SEARCH_PATH + "/*", TYPE_SEARCH);
    uriTemplate.addURI(AUTHORITY, FILE_PATH + "/*", TYPE_FILE);
  }

  //! @return Mime type of uri (used in content provider)
  static String mimeTypeOf(Uri uri) {
    switch (uriTemplate.match(uri)) {
      case TYPE_RESOLVE:
      case TYPE_FILE:
        return MIME_ITEM;
      case TYPE_LISTING:
      case TYPE_SEARCH:
        return MIME_ITEMS_SET;
      case TYPE_PARENTS:
        return MIME_SIMPLE_ITEMS_SET;
      default:
        throw new IllegalArgumentException("Wrong URI: " + uri);
    }
  }

  static Uri getPathFrom(Uri uri) {
    switch (uriTemplate.match(uri)) {
      case TYPE_RESOLVE:
      case TYPE_LISTING:
      case TYPE_PARENTS:
      case TYPE_SEARCH:
      case TYPE_FILE:
        final List<String> segments = uri.getPathSegments();
        return segments.size() > 1 ? Uri.parse(segments.get(1)) : Uri.EMPTY;
      default:
        throw new IllegalArgumentException("Wrong URI: " + uri);
    }
  }

  static String getQueryFrom(Uri uri) {
    if (uriTemplate.match(uri) == TYPE_SEARCH) {
      final String query = uri.getQueryParameter(QUERY_PARAM);
      if (query != null) {
        return query;
      }
    }
    throw new IllegalArgumentException("Wrong search URI: " + uri);
  }

  @Type
  static int getUriType(Uri uri) {
    return uriTemplate.match(uri);
  }

  static Uri resolveUriFor(Uri uri) {
    return makeUri(RESOLVE_PATH, uri).build();
  }

  static Uri listingUriFor(Uri uri) {
    return makeUri(LISTING_PATH, uri).build();
  }

  static Uri parentsUriFor(Uri uri) {
    return makeUri(PARENTS_PATH, uri).build();
  }

  static Uri searchUriFor(Uri uri, String query) {
    return makeUri(SEARCH_PATH, uri).appendQueryParameter(QUERY_PARAM, query).build();
  }

  static Uri fileUriFor(Uri uri) {
    return makeUri(FILE_PATH, uri).build();
  }

  private static Uri.Builder makeUri(String path, Uri uri) {
    return new Uri.Builder()
               .scheme(ContentResolver.SCHEME_CONTENT)
               .authority(AUTHORITY)
               .encodedPath(path)
               .appendPath(uri.toString());
  }
}
