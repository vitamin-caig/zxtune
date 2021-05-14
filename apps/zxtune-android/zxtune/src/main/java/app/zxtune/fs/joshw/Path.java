package app.zxtune.fs.joshw;

import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.Collections;
import java.util.List;

import app.zxtune.fs.api.Cdn;

/*
 * joshw:/${Catalogue}/${FilePath}
 */
public final class Path extends app.zxtune.fs.httpdir.PathBase {

  private static final String SCHEME = "joshw";
  private static final String EMPTY_CATALOGUE = "";

  private static final Path EMPTY = new Path(Collections.EMPTY_LIST, true);

  private Path(List<String> elements, boolean isDir) {
    super(elements, isDir);
  }

  @Override
  public Uri[] getRemoteUris() {
    final String catalogue = getCatalogue();
    final String path = getInnerPath();
    return new Uri[] {
        Cdn.joshw(catalogue, path),
        new Uri.Builder()
        .scheme("http")
        .authority(catalogue + ".joshw.info")
        .path(path)
        .build()
    };
  }

  private String getInnerPath() {
    final String remoteId = getRemoteId();
    return remoteId.substring(remoteId.indexOf('/') + 1);
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
    // roots are always directories
    return new Path(elements, isDir || elements.size() < 2);
  }

  public final boolean isCatalogue() {
    return getElements().size() == 1;
  }

  public final String getCatalogue() {
    return getElements().isEmpty() ? EMPTY_CATALOGUE : getElements().get(0);
  }

  @Nullable
  public static Path parse(Uri uri) {
    if (SCHEME.equals(uri.getScheme())) {
      final String path = uri.getPath();
      return path != null ? (Path) EMPTY.getChild(path) : EMPTY;
    } else {
      return null;
    }
  }

  public static Path create() {
    return EMPTY;
  }
}
