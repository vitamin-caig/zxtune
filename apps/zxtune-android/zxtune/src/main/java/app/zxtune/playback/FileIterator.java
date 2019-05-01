/**
 * @file
 * @brief Implementation of Iterator based on Vfs objects
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.playback;

import android.content.Context;
import android.net.Uri;
import android.support.annotation.NonNull;
import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.core.Identifier;
import app.zxtune.fs.DefaultComparator;
import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.playback.stubs.PlayableItemStub;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.concurrent.LinkedBlockingQueue;

public class FileIterator implements Iterator {

  private static final String TAG = FileIterator.class.getName();

  private static final int MAX_VISITED = 10;

  private final LinkedBlockingQueue<PlayableItem> itemsQueue;
  private final ArrayList<PlayableItem> history;
  private final Exception[] lastError;
  @SuppressWarnings({"FieldCanBeLocal", "unused"})
  private final Object scanHandle;
  private int historyDepth;

  public static FileIterator create(Context ctx, Uri uri) throws Exception {
    final java.util.Iterator<VfsFile> filesIterator = creatDirFilesIterator(uri);
    return new FileIterator(ctx, filesIterator);
  }

  private FileIterator(Context context, @NonNull java.util.Iterator<VfsFile> files) throws Exception {
    this.itemsQueue = new LinkedBlockingQueue<>(2);
    this.history = new ArrayList<>(MAX_VISITED + 1);
    this.lastError = new Exception[1];
    this.scanHandle = startAsyncScanning(files);
    this.historyDepth = 0;
    if (!takeNextItem()) {
      if (lastError[0] != null) {
        throw lastError[0];
      }
      throw new Exception(context.getString(R.string.no_tracks_found));
    }
  }

  @Override
  public PlayableItem getItem() {
    return history.get(historyDepth);
  }

  @Override
  public boolean next() {
    if (0 != historyDepth) {
      --historyDepth;
      return true;
    }
    if (takeNextItem()) {
      while (history.size() > MAX_VISITED) {
        history.remove(MAX_VISITED);
      }
      return true;
    }
    return false;
  }

  @Override
  public boolean prev() {
    if (historyDepth + 1 < history.size()) {
      ++historyDepth;
      return true;
    }
    return false;
  }

  private Object startAsyncScanning(final java.util.Iterator<VfsFile> files) {
    return AsyncScanner.scan(new ScannerCallback(files, itemsQueue, lastError));
  }

  private static class ScannerCallback implements AsyncScanner.Callback {

      private final int[] counter = {0, 0};
      private final java.util.Iterator<VfsFile> files;
      private final LinkedBlockingQueue<PlayableItem> itemsQueue;
      private final Exception[] lastError;

      ScannerCallback(java.util.Iterator<VfsFile> files, LinkedBlockingQueue<PlayableItem> itemsQueue,
                      Exception[] lastError) {
        this.files = files;
        this.itemsQueue = itemsQueue;
        this.lastError = lastError;
      }

      @Override
      public VfsFile getNextFile() {
        final int doneFiles = counter[0]++;
        final int itemsCount = counter[1];
        if ((itemsCount == 0 && doneFiles > 0) || !files.hasNext()) {
          finish();
          return null;
        } else {
          return files.next();
        }
      }

      @Override
      public Reply onItem(@NonNull PlayableItem item) {
        if (itemsQueue.offer(item)) {
          ++counter[1];
          return Reply.CONTINUE;
        } else {
          return Reply.RETRY;
        }
      }

      private void finish() {
        try {
          itemsQueue.put(PlayableItemStub.instance());
        } catch (InterruptedException e) {
          Log.w(TAG, e, "Interrupted Callback.onFinish");
        }
      }

      @Override
      public void onError(Identifier id, Exception e) {
        lastError[0] = e;
      }
    }

  private boolean takeNextItem() {
    try {
      final PlayableItem newItem = itemsQueue.take();
      if (newItem != PlayableItemStub.instance()) {
        history.add(0, newItem);
        return true;
      }
      //put limiter back
      itemsQueue.put(newItem);
    } catch (InterruptedException e) {
      Log.w(TAG, e, "Interrupted takeNextItem");
    }
    return false;
  }

  @NonNull
  private static java.util.Iterator<VfsFile> creatDirFilesIterator(Uri start) throws Exception {
    final ArrayList<VfsFile> result = new ArrayList<>();
    final VfsFile file = (VfsFile) VfsArchive.resolve(start);
    final VfsDir parent = (VfsDir) file.getParent();
    if (parent == null) {
      result.add(file);
      return result.listIterator();
    }
    parent.enumerate(new VfsDir.Visitor() {
      @Override
      public void onItemsCount(int count) {
        result.ensureCapacity(count);
      }

      @Override
      public void onDir(VfsDir dir) {
      }

      @Override
      public void onFile(VfsFile file) {
        result.add(file);
      }
    });
    final Object extension = parent.getExtension(VfsExtensions.COMPARATOR);
    final Comparator<VfsObject> comparator = extension instanceof Comparator<?>
                                                 ? (Comparator<VfsObject>) extension
                                                 : DefaultComparator.instance();
    Collections.sort(result, comparator);
    // Resolved file may be incomparable with dir content due to lack of some properties
    // E.g. track from zxart/Top doesn't have rating info
    // So use plain linear search by uri
    final Uri normalizedUri = file.getUri();
    for (int idx = 0, lim = result.size(); idx < lim; ++idx) {
      final VfsFile cur = result.get(idx);
      final Uri curUri = cur.getUri();
      if (normalizedUri.equals(curUri)) {
        return result.listIterator(idx);
      }
    }
    return result.listIterator();
  }
}
