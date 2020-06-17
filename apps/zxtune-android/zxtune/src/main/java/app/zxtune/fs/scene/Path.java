package app.zxtune.fs.scene;

import android.net.Uri;
import androidx.annotation.Nullable;
import android.text.TextUtils;
import app.zxtune.fs.api.Cdn;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

// not including /music element!!!

// music/{artists,compos,disks,groups/....
public final class Path implements app.zxtune.fs.httpdir.Path {

  private static final String SCHEME = "scene";

  private final List<String> elements;

  private Path(List<String> elements) {
    this.elements = elements;
  }

  @Override
  public Uri[] getRemoteUris() {
    final String path = elements.isEmpty() ? "" : (getLocalId() + (isFile() ? "" : "/"));
    return new Uri[]{
        Cdn.scene(path),
        new Uri.Builder()
            .scheme("https")
            .authority("archive.scene.org")
            .path("pub/music/" + path)
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
    final int size = elements.size();
    if (size < 2) {
      return false;
    }
    final String name = elements.get(size - 1);
    final int dot = name.lastIndexOf('.');
    if (dot == -1) {
      return false;
    }
    final String ext = name.substring(dot + 1);
    return ext.length() <= 3;
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
