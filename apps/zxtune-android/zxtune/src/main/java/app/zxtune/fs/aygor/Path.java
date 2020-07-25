package app.zxtune.fs.aygor;

import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.Collections;
import java.util.List;

/*
 * 1) aygor:/${FilePath} - direct access to files or folders starting from http://abrimaal.pro-e.pl/ayon/ dir
 */
public final class Path extends app.zxtune.fs.httpdir.PathBase {

  private static final String SCHEME = "aygor";

  private static final Path EMPTY = new Path(Collections.EMPTY_LIST, true);

  private Path(List<String> elements, boolean isDir) {
    super(elements, isDir);
  }

  @Override
  public Uri[] getRemoteUris() {
    return new Uri[] {
        new Uri.Builder()
            .scheme("http")
            .authority("abrimaal.pro-e.pl")
            .path("ayon/" + getRemoteId())
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
      String path = uri.getPath();
      return path != null ? (Path) EMPTY.getChild(path) : EMPTY;
    } else {
      return null;
    }
  }

  public static Path create() {
    return EMPTY;
  }
}
