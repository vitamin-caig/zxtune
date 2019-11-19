package app.zxtune.fs.joshw;

import android.net.Uri;
import androidx.annotation.Nullable;
import android.text.TextUtils;
import app.zxtune.fs.api.Cdn;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/*
 * joshw:/${Catalogue}/${FilePath}
 */
public final class Path implements app.zxtune.fs.httpdir.Path {

  private static final String SCHEME = "joshw";

  private final String catalogue;
  private final List<String> elements;

  private Path(String catalogue, List<String> elements) {
    this.catalogue = catalogue;
    this.elements = elements;
  }

  @Override
  public Uri[] getRemoteUris() {
    final String path = elements.isEmpty() ? "" : (TextUtils.join("/", elements) + (isFile() ? "" : "/"));
    return new Uri[] {
        Cdn.joshw(catalogue, path),
        new Uri.Builder()
        .scheme("http")
        .authority(catalogue + ".joshw.info")
        .path(path)
        .build()
    };
  }

  @Override
  public String getLocalId() {
    return catalogue + "/" + TextUtils.join("/", elements);
  }

  @Override
  @Nullable
  public Path getParent() {
    final int count = elements.size();
    switch (count) {
      case 0:
        return TextUtils.isEmpty(catalogue) ? null : new Path(null, Collections.EMPTY_LIST);
      case 1:
        return new Path(catalogue, Collections.EMPTY_LIST);
      default:
        return new Path(catalogue, elements.subList(0, count - 1));
    }
  }

  @Override
  public Uri getUri() {
    final Uri.Builder builder = new Uri.Builder().scheme(SCHEME);
    if (!TextUtils.isEmpty(catalogue)) {
      builder.appendPath(catalogue);
      for (String el : elements) {
        builder.appendPath(el);
      }
    }
    return builder.build();
  }

  @Override
  @Nullable
  public final String getName() {
    final int count = elements.size();
    return count > 0
            ? elements.get(count - 1)
            : catalogue;
  }

  @Override
  public Path getChild(String name) {
    if (isEmpty()) {
      return new Path(name, Collections.EMPTY_LIST);
    } else {
      final ArrayList<String> result = new ArrayList<>(elements.size() + 1);
      result.addAll(elements);
      result.add(name);
      return new Path(catalogue, result);
    }
  }

  @Override
  public final boolean isEmpty() {
    return TextUtils.isEmpty(catalogue);
  }

  @Override
  public final boolean isFile() {
    return isFileName(getName());
  }

  private static boolean isFileName(@Nullable String component) {
    return component != null && component.toLowerCase().endsWith(".7z");
  }

  public final boolean isCatalogue() {
    return elements.isEmpty() && !TextUtils.isEmpty(catalogue);
  }

  public final String getCatalogue() {
    return catalogue;
  }

  @Nullable
  public static Path parse(Uri uri) {
    if (SCHEME.equals(uri.getScheme())) {
      final List<String> elements = uri.getPathSegments();
      final int count = elements.size();
      switch (count) {
        case 0:
          return new Path(null, Collections.EMPTY_LIST);
        case 1:
          return new Path(elements.get(0), Collections.EMPTY_LIST);
        default:
          final String catalogue = elements.get(0);
          //compatibility
          final ArrayList<String> decoded = new ArrayList<>(count - 1);
          for (int idx = 1; idx < count; ++idx) {
            decoded.add(Uri.decode(elements.get(idx)));
          }
          return new Path(catalogue, decoded);
      }
    } else {
      return null;
    }
  }

  public static Path create() {
    return new Path(null, Collections.EMPTY_LIST);
  }
}
