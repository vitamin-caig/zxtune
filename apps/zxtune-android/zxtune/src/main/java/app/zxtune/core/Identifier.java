package app.zxtune.core;

import android.net.Uri;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;

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
  @NonNull
  private final String subpath;

  public Identifier(@NonNull Uri location, @Nullable String subpath) {
    this.fullpath = withSubpath(location, subpath);
    this.location = location;
    this.subpath = subpath != null ? subpath : "";
  }

  public Identifier(@NonNull Uri fullpath) {
    this.fullpath = fullpath;
    this.location = withSubpath(fullpath, "");
    this.subpath = getSubpath(fullpath);
  }

  @NonNull
  public static Identifier parse(@Nullable String str) {
    return str != null
            ? new Identifier(Uri.parse(str))
        : EMPTY;
  }

  private static Uri withSubpath(Uri in, String subpath) {
    final Uri.Builder builder = in.buildUpon();
    builder.fragment(subpath);
    return builder.build();
  }

  @NonNull
  private static String getSubpath(Uri in) {
    final String fragment = in.getFragment();
    return fragment != null ? fragment : "";
  }

  @NonNull
  public final Uri getFullLocation() {
    return fullpath;
  }

  @NonNull
  public final Uri getDataLocation() {
    return location;
  }

  @NonNull
  public final String getSubpath() {
    return subpath;
  }

  @NonNull
  public final String getDisplayFilename() {
    final String filename = location.getLastPathSegment();
    if (subpath.length() == 0) {
      return filename != null ? filename : "";
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
    if (rh instanceof Identifier) {
      final Identifier id = (Identifier) rh;
      return fullpath.equals(id.fullpath);
    } else {
      return false;
    }
  }
}
