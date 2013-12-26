/**
 *
 * @file
 *
 * @brief Recursive iterator helper
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs;

import java.io.IOException;
import java.util.LinkedList;

import android.content.Context;
import android.net.Uri;
import android.util.Log;

public final class VfsIterator {

  private static final String TAG = VfsIterator.class.getName();
  
  private final LinkedList<VfsFile> files;
  private final LinkedList<VfsDir> dirs;

  public VfsIterator(Context context, Uri[] paths) throws IOException {
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
      scan(dirs.remove());
    }
  }
  
  private void scan(VfsDir root) {
    try {
      final LinkedList<VfsDir> newDirs = new LinkedList<VfsDir>();
      final LinkedList<VfsFile> newFiles = new LinkedList<VfsFile>();
      root.enumerate(new VfsDir.Visitor() {
        @Override
        public void onDir(VfsDir dir) {
          newDirs.add(dir);
        }
  
        @Override
        public void onFile(VfsFile file) {
          newFiles.add(file);
        }
      });
      //move to depth first
      dirs.addAll(0, newDirs);
      files.addAll(newFiles);
    } catch (IOException e) {
      Log.d(TAG, "Skip I/O error", e);
    }
  }

  public final VfsFile getFile() {
    return files.element();
  }
}
