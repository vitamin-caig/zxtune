/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs;

import java.util.ArrayList;
import java.util.LinkedList;

import android.content.Context;
import android.net.Uri;

public final class VfsIterator {

  private final ArrayList<VfsFile> visitedFiles;
  private final LinkedList<VfsDir> unvisitedDirs;
  private int index;

  public VfsIterator(Context context, Uri path) {
    this.visitedFiles = new ArrayList<VfsFile>();
    this.unvisitedDirs = new LinkedList<VfsDir>();
    final VfsRoot root = Vfs.createRoot(context);
    final VfsObject obj = root.resolve(path);
    if (obj == null) {
      return;
    }
    if (obj instanceof VfsFile) {
      visitedFiles.add((VfsFile) obj);
    } else if (obj instanceof VfsDir) {
      unvisitedDirs.add((VfsDir) obj);
    }
    loadNextDir();
  }

  public final boolean isValid() {
    return 0 <= index && index < visitedFiles.size();
  }
  
  public final int getPos() {
    return index;
  }
  
  public final void setPos(int pos) {
    index = pos;
    loadNextDir();
  }
  
  public final void next() {
    ++index;
    loadNextDir();
  }

  public void prev() {
    --index;
  }

  public final VfsFile getFile() {
    return isValid() ? visitedFiles.get(index) : null;
  }
  
  private void loadNextDir() {
    while (index >= visitedFiles.size() && !unvisitedDirs.isEmpty()) {
      unvisitedDirs.pop().enumerate(new VfsDir.Visitor() {
        @Override
        public Status onDir(VfsDir dir) {
          unvisitedDirs.push(dir);
          return Status.CONTINUE;
        }

        @Override
        public Status onFile(VfsFile file) {
          visitedFiles.add(file);
          return Status.CONTINUE;
        }
      });
    }
  }
}
