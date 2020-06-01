package app.zxtune.core;

import android.net.Uri;

import androidx.annotation.Nullable;

public class Identifier {

  private static final String SUBPATH_DELIMITER = "/";
  public static final Identifier EMPTY = new Identifier(Uri.EMPTY);

  /**
   * Identifier is not fully compatible with playlists from desktop version of zxtune
   *
   * E.g. first track in ay file is stored as test.ay?#1 without hash sign encoding
   */
  private final Uri fullpath;
  private final Uri location;
  private final String subpath;

  public Identifier(Uri location, @Nullable String subpath) {
    this.fullpath = withSubpath(location, subpath);
    this.location = location;
    this.subpath = subpath != null ? subpath : "";
  }

  public Identifier(Uri fullpath) {
    this.fullpath = fullpath;
    this.location = withSubpath(fullpath, "");
    this.subpath = getSubpath(fullpath);
  }

  public static Identifier parse(@Nullable String str) {
    return str != null
            ? new Identifier(Uri.parse(str))
        : EMPTY;
  }

  private static Uri withSubpath(Uri in, @Nullable String subpath) {
    final Uri.Builder builder = in.buildUpon();
    if (subpath != null) {
      builder.fragment(subpath);
    }
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

  public final String getDisplayFilename() {
    final String filename = location.getLastPathSegment();
    if (subpath.length() == 0) {
      return filename != null ? filename : "";
    } else {
      final String subname = subpath.substring(subpath.lastIndexOf(SUBPATH_DELIMITER) + 1);
      return filename + " > " + subname;
    }
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
    if (rh instanceof Identifier) {
      final Identifier id = (Identifier) rh;
      return fullpath.equals(id.fullpath);
    } else {
      return false;
    }
  }
}
