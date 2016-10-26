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

import java.util.ArrayList;

import android.net.Uri;

import app.zxtune.Log;

final class VfsRootComposite extends StubObject implements VfsRoot {

  private static final String TAG = VfsRootComposite.class.getName();
  
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
  public VfsObject resolve(Uri uri) {
    if (Uri.EMPTY.equals(uri)) {
      return this;
    }
    for (VfsRoot root : subRoots) {
      try {
        final VfsObject obj = root.resolve(uri);
        if (obj != null) {
          return obj;
        }
      } catch (Exception e) {
        Log.w(TAG, e, "Failed to resolve %s", uri.toString());
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
