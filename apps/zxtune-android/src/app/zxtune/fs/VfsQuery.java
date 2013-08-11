/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs;

import android.content.ContentResolver;
import android.net.Uri;
import android.text.TextUtils;

/*
 * content://app.zxtune.vfs/${path} - list all items at decode(${path}) 
 */

public class VfsQuery {
  
  public static final class Columns {
    public static final int ID = 0;
    public static final int TYPE = 1;
    public static final int NAME = 2;
    public static final int URI = 3;
    public static final int SIZE = 4;
    
    public static final int TOTAL = 5;
  }
  
  public static final class Types {
    public static final int FILE = 0;
    public static final int DIR = 1;
  }
  
  private static final class QueryType {

    public final String mimeType;
    public final int uriType;
    
    QueryType(String mime) {
      this.mimeType = mime;
      this.uriType = Math.abs(mimeType.hashCode());
    }
  }
  
  private static final String AUTHORITY = "app.zxtune.vfs";
  
  private static final QueryType QUERY_DIR;
  
  static {
    QUERY_DIR = new QueryType("vnd.android.cursor.dir/vnd." + AUTHORITY);
  }
  
  private final QueryType type;
  private final Uri path;

  public VfsQuery(Uri uri) {
    //UriMatcher does not work with empty paths
    if (uri.getAuthority().equals(AUTHORITY)) {
      this.type = QUERY_DIR;
      final String path = Uri.decode(uri.getLastPathSegment());
      this.path = TextUtils.isEmpty(path) ? Uri.EMPTY : Uri.parse(path);
    } else {
      throw new IllegalArgumentException("Wrong URI: " + uri);
    }
  }

  public Uri getPath() {
    return path;
  }
  
  public String getMimeType() {
    return type.mimeType;
  }
  
  public static Uri unparse(Uri path) {
    final Uri.Builder builder = new Uri.Builder()
      .scheme(ContentResolver.SCHEME_CONTENT)
      .authority(AUTHORITY)
      .path(Uri.encode(path.toString()));
    return builder.build();
  }
}
