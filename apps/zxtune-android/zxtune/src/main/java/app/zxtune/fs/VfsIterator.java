/**
 * @file
 * @brief Recursive iterator helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import java.io.IOException;
import java.util.ArrayDeque;

public final class VfsIterator {

  public interface ErrorHandler {
    void onIOError(IOException e);
  }

  private final ErrorHandler handler;
  private final ArrayDeque<VfsFile> files;
  private final ArrayDeque<VfsDir> dirs;

  public VfsIterator(VfsDir dir, ErrorHandler handler) {
    this.handler = handler;
    this.files = new ArrayDeque<>();
    this.dirs = new ArrayDeque<>();
    this.dirs.add(dir);
    prefetch();
  }

  public boolean isValid() {
    return !files.isEmpty();
  }

  public void next() {
    files.remove();
    prefetch();
  }

  public VfsFile getFile() {
    return files.element();
  }

  private void prefetch() {
    while (files.isEmpty()) {
      if (!dirs.isEmpty()) {
        scan(dirs.remove());
      } else {
        break;
      }
    }
  }

  private void scan(VfsDir root) {
    try {
      root.enumerate(new VfsDir.Visitor() {
        @Override
        public void onDir(VfsDir dir) {
          //move to depth first
          //assume that sorting is not sensitive
          dirs.addFirst(dir);
        }

        @Override
        public void onFile(VfsFile file) {
          files.addLast(file);
        }
      });
    } catch (IOException e) {
      handler.onIOError(e);
    }
  }
}
