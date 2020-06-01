package app.zxtune.fs.aminet;

import android.net.Uri;
import androidx.annotation.Nullable;
import android.text.TextUtils;
import app.zxtune.fs.api.Cdn;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

// not including /mods element!!!

// mods/${author}/${file}
public final class Path implements app.zxtune.fs.httpdir.Path {

  private static final String SCHEME = "aminet";

  private final List<String> elements;

  private Path(List<String> elements) {
    this.elements = elements;
  }

  @Override
  public Uri[] getRemoteUris() {
    final String path = getLocalId();
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
  public String getLocalId() {
    return elements.isEmpty() ? "" : TextUtils.join("/", elements);
  }

  @Override
  public Uri getUri() {
    return new Uri.Builder()
               .scheme(SCHEME)
               .path(getLocalId())
               .build();
  }

  @Override
  public String getName() {
    final int count = elements.size();
    return count > 0
               ? elements.get(count - 1)
               : "";
  }

  @Nullable
  @Override
  public Path getParent() {
    final int count = elements.size();
    switch (count) {
      case 0:
        return null;
      case 1:
        return new Path(Collections.EMPTY_LIST);
      default:
        return new Path(elements.subList(0, count - 1));
    }
  }

  @Override
  public Path getChild(String name) {
    final ArrayList<String> result = new ArrayList<>(elements.size() + 1);
    if (name.startsWith("/")) {
      //absolute path
      name = name.substring(1);
    } else {
      result.addAll(elements);
    }
    if (name.contains("/")) {
      Collections.addAll(result, name.split("/"));
    } else {
      result.add(name);
    }
    return new Path(result);
  }

  @Override
  public boolean isEmpty() {
    return elements.isEmpty();
  }

  @Override
  public boolean isFile() {
    return elements.size() == 2;
  }

  @Nullable
  public static Path parse(Uri uri) {
    if (SCHEME.equals(uri.getScheme())) {
      return new Path(uri.getPathSegments());
    } else {
      return null;
    }
  }

  public static Path create() {
    return new Path(Collections.EMPTY_LIST);
  }
}
