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

public class Identifier {

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
    this.subpath = subpath;
  }

  public Identifier(Uri fullpath) {
    this.fullpath = fullpath;
    this.location = withSubpath(fullpath, "");
    this.subpath = getSubpath(fullpath);
  }
  
  private static Uri withSubpath(Uri in, String subpath) {
    final Uri.Builder builder = in.buildUpon();
    builder.fragment(subpath);
    return builder.build();
  }
  
  private static String getSubpath(Uri in) {
    final String fragment = in.getFragment();
    return fragment;
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
