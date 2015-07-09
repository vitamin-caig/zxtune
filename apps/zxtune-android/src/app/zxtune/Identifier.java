/**
 * 
 * @file
 *
 * @brief Module identifier helper
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune;

import android.net.Uri;
import android.text.TextUtils;

public class Identifier {
  
  private final static String SUBPATH_DELIMITER = "/";
  public final static Identifier EMPTY = new Identifier(Uri.EMPTY);

  /**
   * Identifier is not fully compatible with playlists from desktop version of zxtune
   * 
   * E.g. first track in ay file is stored as test.ay?#1 without hash sign encoding
   */
  private final Uri fullpath;
  private final Uri location;
  private final String subpath;
  
  public Identifier(Uri location, String subpath) {
    this.fullpath = withSubpath(location, subpath);
    this.location = location;
    this.subpath = subpath != null ? subpath : "";
  }

  public Identifier(Uri fullpath) {
    this.fullpath = fullpath;
    this.location = withSubpath(fullpath, "");
    this.subpath = getSubpath(fullpath);
  }
  
  public static Identifier parse(String str) {
    return new Identifier(Uri.parse(str));
  }
  
  private static Uri withSubpath(Uri in, String subpath) {
    final Uri.Builder builder = in.buildUpon();
    builder.fragment(subpath);
    return builder.build();
  }
  
  private static String getSubpath(Uri in) {
    final String fragment = in.getFragment();
    return fragment != null ? fragment : "";
  }
  
  public final Uri getFullLocation() {
    return fullpath;
  }

  public final Uri getDataLocation() {
    return location;
  }
  
  public final String getSubpath() {
    return subpath;
  }
  
  public final Identifier withSubpath(String subpath) {
    return new Identifier(location, subpath);
  }
  
  public final Identifier withSubpath(String[] components) {
    return withSubpath(TextUtils.join(SUBPATH_DELIMITER, components));
  }
  
  public final String getDisplayFilename() {
    final String filename = location.getLastPathSegment();
    if (subpath.length() == 0) {
      return filename;
    } else {
      final String subname = subpath.substring(subpath.lastIndexOf(SUBPATH_DELIMITER) + 1);
      return filename + " > " + subname;  
    }
  }
  
  public final String[] getSubpathComponents() {
    return TextUtils.split(subpath, SUBPATH_DELIMITER);
  }
  
  @Override
  public String toString() {
    return fullpath.toString();
  }
  
  @Override
  public int hashCode() {
    return fullpath.hashCode();
  }
  
  @Override
  public boolean equals(Object rh) {
    final Identifier id = (Identifier) rh;
    return fullpath.equals(id.fullpath);
  }
}
