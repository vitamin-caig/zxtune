package app.zxtune.core;

import android.net.Uri;
import android.text.TextUtils;

import androidx.annotation.Nullable;

public class Identifier {

  private static final String SUBPATH_DELIMITER = "/";
  public static final Identifier EMPTY = new Identifier(Uri.EMPTY);

  /**
   * Identifier is not fully compatible with playlists from desktop version of zxtune
   * <p>
   * E.g. first track in ay file is stored as test.ay?#1 without hash sign encoding
   */
  private final Uri fullPath;
  private final Uri location;
  private final String subPath;

  public Identifier(Uri location, @Nullable String subPath) {
    final Uri.Builder builder = location.buildUpon();
    builder.fragment(null);
    this.location = builder.build();
    if (TextUtils.isEmpty(this.location.getPath())) {
      this.fullPath = this.location;
      this.subPath = "";
    } else {
      builder.fragment(subPath);
      this.fullPath = builder.build();
      String fragment = fullPath.getFragment();
      this.subPath = fragment != null ? fragment : "";
    }
  }

  public Identifier(Uri fullPath) {
    this(fullPath, fullPath.getFragment());
  }

  public static Identifier parse(@Nullable String str) {
    return str != null
            ? new Identifier(Uri.parse(str))
            : EMPTY;
  }

  public final Uri getFullLocation() {
    return fullPath;
  }

  public final Uri getDataLocation() {
    return location;
  }

  public final String getSubPath() {
    return subPath;
  }

  public final String getDisplayFilename() {
    final String filename = location.getLastPathSegment();
    if (subPath.length() == 0) {
      return filename != null ? filename : "";
    } else {
      final String subName = subPath.substring(subPath.lastIndexOf(SUBPATH_DELIMITER) + 1);
      return filename + " > " + subName;
    }
  }

  @Override
  public String toString() {
    return fullPath.toString();
  }

  @Override
  public int hashCode() {
    return fullPath.hashCode();
  }

  @Override
  public boolean equals(Object rh) {
    if (rh instanceof Identifier) {
      final Identifier id = (Identifier) rh;
      return fullPath.equals(id.fullPath);
    } else {
      return false;
    }
  }
}
