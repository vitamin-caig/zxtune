/**
 * @file
 * @brief Composite root helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.net.Uri;

import androidx.annotation.Nullable;
import androidx.core.util.ObjectsCompat;

import java.util.ArrayList;

import app.zxtune.Log;

class VfsRootComposite extends StubObject implements VfsRoot {

  private static final String TAG = VfsRootComposite.class.getName();

  private final String scheme;
  private final Uri uri;
  private final ArrayList<VfsRoot> subRoots;

  VfsRootComposite(String scheme) {
    this.scheme = scheme;
    this.uri = scheme != null ? Uri.fromParts(scheme, "", "") : Uri.EMPTY;
    this.subRoots = new ArrayList<>();
  }

  final void addSubroot(VfsRoot root) {
    subRoots.add(root);
  }

  @Override
  @Nullable
  public VfsObject getParent() {
    return null;
  }

  @Override
  @Nullable
  public VfsObject resolve(Uri uri) {
    if (matchScheme(uri.getScheme())) {
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

  private boolean matchScheme(String scheme) {
    return ObjectsCompat.equals(this.scheme, scheme);
  }

  @Override
  public void enumerate(Visitor visitor) {
    for (VfsRoot root : subRoots) {
      visitor.onDir(root);
    }
  }

  @Override
  public Uri getUri() {
    return uri;
  }

  @Override
  public String getName() {
    return getDescription();
  }
}
