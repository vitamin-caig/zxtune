/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs;

import java.util.LinkedList;

import android.content.Context;
import android.net.Uri;

public final class VfsIterator {

  private final LinkedList<VfsFile> files;
  private final LinkedList<VfsDir> dirs;

  public VfsIterator(Context context, Uri[] paths) {
    this.files = new LinkedList<VfsFile>();
    this.dirs = new LinkedList<VfsDir>();
    final VfsRoot root = Vfs.createRoot(context);
    for (Uri path : paths) {
      final VfsObject obj = root.resolve(path);
      if (obj == null) {
        continue;
      }
      if (obj instanceof VfsFile) {
        files.add((VfsFile) obj);
      } else if (obj instanceof VfsDir) {
        dirs.add((VfsDir) obj);
      }
    }
    files.add(0, null);//stub head
    next();
  }

  public final boolean isValid() {
    return !files.isEmpty();
  }
  
  public final void next() {
    files.remove();
    while (files.isEmpty() && !dirs.isEmpty()) {
      dirs.remove().enumerate(new VfsDir.Visitor() {
        @Override
        public Status onDir(VfsDir dir) {
          //move depth
          dirs.addFirst(dir);
          return Status.CONTINUE;
        }

        @Override
        public Status onFile(VfsFile file) {
          files.add(file);
          return Status.CONTINUE;
        }
      });
    }
  }

  public final VfsFile getFile() {
    return files.element();
  }
}
