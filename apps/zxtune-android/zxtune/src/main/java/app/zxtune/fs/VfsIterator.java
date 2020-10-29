/**
 * @file
 * @brief Recursive iterator helper
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.net.Uri;

import java.io.IOException;
import java.util.ArrayDeque;
import java.util.Collections;

import app.zxtune.Log;

public final class VfsIterator {

  public interface ErrorHandler {
    void onIOError(IOException e);
  }

  private static final String TAG = VfsIterator.class.getName();

  private final ErrorHandler handler;
  private final ArrayDeque<VfsFile> files;
  private final ArrayDeque<VfsDir> dirs;
  private final ArrayDeque<Uri> paths;

  public VfsIterator(Uri[] paths) {
    this(paths, e -> Log.w(TAG, e, "Skip I/O error"));
  }

  public VfsIterator(Uri[] paths, ErrorHandler handler) {
    this.handler = handler;
    this.files = new ArrayDeque<>();
    this.dirs = new ArrayDeque<>();
    this.paths = new ArrayDeque<>(paths.length);
    Collections.addAll(this.paths, paths);
    prefetch();
  }

  public final boolean isValid() {
    return !files.isEmpty();
  }

  public final void next() {
    files.remove();
    prefetch();
  }

  public final VfsFile getFile() {
    return files.element();
  }

  private void prefetch() {
    while (files.isEmpty()) {
      if (!dirs.isEmpty()) {
        scan(dirs.remove());
      } else if (!paths.isEmpty()) {
        resolve(paths.remove());
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

  private void resolve(Uri path) {
    try {
      final VfsObject obj = VfsArchive.resolve(path);
      if (obj instanceof VfsFile) {
        files.addLast((VfsFile) obj);
      } else if (obj instanceof VfsDir) {
        dirs.addLast((VfsDir) obj);
      }
    } catch (IOException e) {
      handler.onIOError(e);
    }
  }
}
