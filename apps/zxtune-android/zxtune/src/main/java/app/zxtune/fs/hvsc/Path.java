package app.zxtune.fs.hvsc;

import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.Collections;
import java.util.List;

import app.zxtune.fs.api.Cdn;

/*
 * 1) hvsc:/C64Music/${FilePath} - deprecated, compatibility
 * 2) hvsc:/${FilePath} - starting from C64Music directory
 */
public final class Path extends app.zxtune.fs.httpdir.PathBase {

  private static final String SCHEME = "hvsc";
  private static final String OLD_PREFIX = "/C64Music/";

  private static final Path EMPTY = new Path(Collections.EMPTY_LIST, true);

  private Path(List<String> elements, boolean isDir) {
    super(elements, isDir);
  }

  @Override
  public Uri[] getRemoteUris() {
    final String path = getRemoteId();
    return new Uri[]{
        Cdn.hvsc(path),
        new Uri.Builder()
            .scheme("https")
            .authority("www.prg.dtu.dk")
            .path("HVSC/C64Music/" + path)
            .build(),
        new Uri.Builder()
            .scheme("http")
            .authority("www.c64.org")
            .path("HVSC/" + path)
            .build()

    };
  }

  @Override
  public Uri getUri() {
    return new Uri.Builder()
        .scheme(SCHEME)
        .path(getRemoteId())
        .build();
  }

  @Override
  public Path build(List<String> elements, boolean isDir) {
    return elements.isEmpty() ? EMPTY : new Path(elements, isDir);
  }

  @Nullable
  public static Path parse(Uri uri) {
    if (SCHEME.equals(uri.getScheme())) {
      String path = stripOldPrefix(uri.getPath());
      return path != null ? (Path) EMPTY.getChild(path) : EMPTY;
    } else {
      return null;
    }
  }

  public static Path create() {
    return EMPTY;
  }

  @Nullable
  private static String stripOldPrefix(@Nullable String path) {
    if (path != null && path.startsWith(OLD_PREFIX)) {
      return path.substring(OLD_PREFIX.length() - 1);
    } else {
      return path;
    }
  }
}
