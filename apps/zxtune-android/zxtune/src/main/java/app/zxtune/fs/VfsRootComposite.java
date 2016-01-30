/**
 *
 * @file
 *
 * @brief Composite root helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs;

import java.io.IOException;
import java.util.ArrayList;

import android.net.Uri;

final class VfsRootComposite extends StubObject implements VfsRoot {
  
  private final ArrayList<VfsRoot> subRoots;
  
  VfsRootComposite() {
    subRoots = new ArrayList<VfsRoot>();
  }

  final void addSubroot(VfsRoot root) {
    subRoots.add(root);
  }
  
  @Override
  public VfsObject getParent() {
    return null;
  }

  @Override
  public VfsObject resolve(Uri uri) throws IOException {
    if (Uri.EMPTY.equals(uri)) {
      return this;
    }
    for (VfsRoot root : subRoots) {
      final VfsObject obj = root.resolve(uri);
      if (obj != null) {
        return obj;
      }
    }
    return null;
  }

  @Override
  public void enumerate(Visitor visitor) {
    for (VfsRoot root : subRoots) {
      visitor.onDir(root);
    }
  }

  @Override
  public Uri getUri() {
    return Uri.EMPTY;
  }

  @Override
  public String getName() {
    return getDescription();
  }
}
