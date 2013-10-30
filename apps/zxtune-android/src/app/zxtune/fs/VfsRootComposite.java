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

final class VfsRootComposite implements VfsRoot {
  
  private final ArrayList<VfsRoot> subRoots;
  
  VfsRootComposite() {
    subRoots = new ArrayList<VfsRoot>();
  }

  final void addSubroot(VfsRoot root) {
    subRoots.add(root);
  }
  
  @Override
  public VfsDir getParent() {
    return null;
  }

  @Override
  public VfsObject resolve(Uri uri) throws IOException {
    if (uri.equals(Uri.EMPTY)) {
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
      if (Visitor.Status.STOP == visitor.onDir(root)) {
        break;
      }
    }
  }

  @Override
  public void find(String mask, Visitor visitor) {
    final VisitorWrapper wrapper = new VisitorWrapper(visitor);
    for (VfsRoot root : subRoots) {
      root.find(mask, wrapper);
      if (wrapper.isStopped()) {
        break;
      }
    }
  }

  @Override
  public Uri getUri() {
    return Uri.EMPTY;
  }

  @Override
  public String getName() {
    return "".intern();
  }

  @Override
  public String getDescription() {
    return getName();
  }

  
  private static class VisitorWrapper implements Visitor {
    
    private final Visitor delegate;
    private boolean stopped;
    
    VisitorWrapper(Visitor delegate) {
      this.delegate = delegate;
      this.stopped = false;
    } 
     
    @Override
    public Status onDir(VfsDir dir) {
      return processStatus(delegate.onDir(dir));
    }
    
    @Override
    public Status onFile(VfsFile file) {
      return processStatus(delegate.onFile(file));
    }
    
    private Status processStatus(Status in) {
      if (in == Status.STOP) {
        stopped = true;
      }
      return in;
    }
    
    public boolean isStopped() {
      return stopped;
    }
  }
}
