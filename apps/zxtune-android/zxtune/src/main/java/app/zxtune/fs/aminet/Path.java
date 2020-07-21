package app.zxtune.fs.aminet;

import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.Collections;
import java.util.List;

import app.zxtune.fs.api.Cdn;

// not including /mods element!!!

// mods/${author}/${file}
public final class Path extends app.zxtune.fs.httpdir.PathBase {

  private static final String SCHEME = "aminet";

  private static final Path EMPTY = new Path(Collections.EMPTY_LIST, true);

  private Path(List<String> elements, boolean isDir) {
    super(elements, isDir);
  }

  @Override
  public Uri[] getRemoteUris() {
    final String path = getRemoteId();
    return new Uri[]{
        Cdn.aminet(path),
        new Uri.Builder()
            .scheme("http")
            .authority("aminet.net")
            .path("mods/" + path)
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
