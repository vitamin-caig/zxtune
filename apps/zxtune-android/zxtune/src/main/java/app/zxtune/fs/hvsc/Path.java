package app.zxtune.fs.hvsc;

import android.net.Uri;
import androidx.annotation.Nullable;
import android.text.TextUtils;
import app.zxtune.fs.api.Cdn;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/*
 * 1) hvsc:/C64Music/${FilePath} - deprecated, compatibility
 * 2) hvsc:/${FilePath} - starting from C64Music directory
 */
public final class Path implements app.zxtune.fs.httpdir.Path {

  private static final String SCHEME = "hvsc";

  private final List<String> elements;

  private Path(List<String> elements) {
    this.elements = elements;
  }

  @Override
  public Uri[] getRemoteUris() {
    final String path = elements.isEmpty() ? "" : (getLocalId() + (isFile() ? "" : "/"));
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
  public String getLocalId() {
    return TextUtils.join("/", elements);
  }

  @Override
  @Nullable
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

  @Override
  public Path getChild(String name) {
    final ArrayList<String> result = new ArrayList<>(elements.size() + 1);
    result.addAll(elements);
    result.add(name);
    return new Path(result);
  }

  @Override
  public boolean isEmpty() {
    return elements.isEmpty();
  }

  @Override
  public boolean isFile() {
    return isFileName(getName());
  }

  private static boolean isFileName(@Nullable String component) {
    return component != null && component.toLowerCase().endsWith(".sid");
  }

  @Nullable
  public static Path parse(Uri uri) {
    if (SCHEME.equals(uri.getScheme())) {
      final List<String> segments = uri.getPathSegments();
      if (isOldFormatPath(segments)) {
        return new Path(segments.subList(1, segments.size()));
      } else {
        return new Path(segments);
      }
    } else {
      return null;
    }
  }

  private static boolean isOldFormatPath(List<String> segments) {
    return segments.size() > 1 && "C64Music".equals(segments.get(0));
  }

  public static Path create() {
    return new Path(Collections.EMPTY_LIST);
  }
}
