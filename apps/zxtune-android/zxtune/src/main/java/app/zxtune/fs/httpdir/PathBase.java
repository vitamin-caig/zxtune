package app.zxtune.fs.httpdir;

import android.text.TextUtils;

import androidx.annotation.Nullable;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public abstract class PathBase implements Path {

  protected final List<String> elements;
  protected final boolean isDir;

  protected PathBase(List<String> elements, boolean isDir) {
    this.elements = elements;
    this.isDir = isDir;
  }

  protected String getRemoteId() {
    final String local = getLocalId();
    return isDir && !local.isEmpty() ? local + "/" : local;
  }

  protected abstract Path build(List<String> elements, boolean isDir);

  @Override
  public String getLocalId() {
    return TextUtils.join("/", elements);
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
        return build(Collections.EMPTY_LIST, true);
      default:
        return build(elements.subList(0, count - 1), true);
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
    return build(result, name.endsWith("/"));
  }

  @Override
  public boolean isEmpty() {
    return elements.isEmpty();
  }

  @Override
  public boolean isFile() {
    return !isDir;
  }
}
