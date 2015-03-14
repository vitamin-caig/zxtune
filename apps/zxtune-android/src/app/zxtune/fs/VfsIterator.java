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
import java.util.ArrayDeque;

import android.net.Uri;
import android.util.Log;

public final class VfsIterator {
  
  public interface ErrorHandler {
    public void onIOError(IOException e);
  }
  
  public static class KeepLastErrorHandler implements ErrorHandler {
    
    private IOException lastError;
    
    @Override
    public void onIOError(IOException e) {
      lastError = e;
    }
    
    public final void throwLastIOError() throws IOException {
      if (lastError != null) {
        throw lastError;
      }
    }
  }
  
  private static final String TAG = VfsIterator.class.getName();

  private final ErrorHandler handler;
  private final ArrayDeque<VfsFile> files;
  private final ArrayDeque<VfsDir> dirs;
  private final ArrayDeque<Uri> paths;
  
  public VfsIterator(Uri[] paths) {
    this(paths, new ErrorHandler() {
      @Override
      public void onIOError(IOException e) {
        Log.d(TAG, "Skip I/O error", e);
      }
    });
  }

  public VfsIterator(Uri[] paths, ErrorHandler handler) {
    this.handler = handler;
    this.files = new ArrayDeque<VfsFile>();
    this.dirs = new ArrayDeque<VfsDir>();
    this.paths = new ArrayDeque<Uri>(paths.length);
    for (Uri path : paths) {
      this.paths.add(path);
    }
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
        public void onItemsCount(int count) {
        }

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
      final VfsObject obj = Vfs.getRoot().resolve(path);
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
