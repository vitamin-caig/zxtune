/**
 * @file
 * @brief Implementation of Iterator based on Vfs objects
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.playback;

import android.content.Context;
import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.concurrent.LinkedBlockingQueue;

import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.core.Identifier;
import app.zxtune.core.Module;
import app.zxtune.core.Scanner;
import app.zxtune.fs.DefaultComparator;
import app.zxtune.fs.VfsArchive;
import app.zxtune.fs.VfsDir;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsFile;
import app.zxtune.fs.VfsObject;
import app.zxtune.playback.stubs.PlayableItemStub;
import app.zxtune.utils.StubProgressCallback;

// TODO: support cleanup of iterator itself
public class FileIterator implements Iterator {

  private static final String TAG = FileIterator.class.getName();

  private static final long MAX_USED_MEMORY = 128 << 20;

  private static class OnDemandUpdateItem {

    private final Identifier location;
    @Nullable
    private PlayableItem cached;
    private final long dataSize;

    OnDemandUpdateItem(PlayableItem delegate) {
      this.location = delegate.getDataId();
      this.cached = delegate;
      this.dataSize = delegate.getModule().getProperty("Size", 0);
    }

    final long getUsedMemory() {
      return cached != null ? dataSize : 0;
    }

    final PlayableItem capture() {
      final PlayableItem result = cached;
      cached = null;
      return result;
    }

    final void preload() {
      if (cached == null) {
        Scanner.analyzeIdentifier(location, new Scanner.Callback() {
          @Override
          public void onModule(Identifier id, Module module) {
            cached = new AsyncScanner.FileItem(id, module);
          }

          @Override
          public void onError(Identifier id, Exception e) {
            Log.w(TAG, e, "Failed to reopen" + id);
          }
        });
      }
    }
  }

  private final LinkedBlockingQueue<PlayableItem> itemsQueue;
  private final ArrayList<OnDemandUpdateItem> history;
  private final Exception[] lastError;
  private long totalUsedMemory;
  @SuppressWarnings({"FieldCanBeLocal", "unused"})
  private final Object scanHandle;
  private int historyDepth;

  public static FileIterator create(Context ctx, Uri uri) throws Exception {
    final java.util.Iterator<VfsFile> filesIterator = creatDirFilesIterator(uri);
    if (!filesIterator.hasNext()) {
      throw new Exception(ctx.getString(R.string.failed_resolve, uri.toString()));
    }
    return new FileIterator(ctx, filesIterator);
  }

  private FileIterator(Context context, java.util.Iterator<VfsFile> files) throws Exception {
    this.itemsQueue = new LinkedBlockingQueue<>(1);
    this.history = new ArrayList<>(100);
    this.lastError = new Exception[1];
    this.totalUsedMemory = 0;
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
    final OnDemandUpdateItem wrapper = history.get(historyDepth);
    totalUsedMemory -= wrapper.getUsedMemory();
    wrapper.preload();
    return wrapper.capture();
  }

  @Override
  public boolean next() {
    if (0 != historyDepth) {
      --historyDepth;
      return true;
    }
    return takeNextItem();
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

      @Nullable
      @Override
      public VfsFile getNextFile() {
        final int doneFiles = counter[0];
        counter[0]++;
        final int itemsCount = counter[1];
        if ((itemsCount == 0 && doneFiles > 0) || !files.hasNext()) {
          finish();
          return null;
        } else {
          return files.next();
        }
      }

      @Override
      public Reply onItem(PlayableItem item) {
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
        addItem(newItem);
        return true;
      }
      //put limiter back
      itemsQueue.put(newItem);
    } catch (InterruptedException e) {
      Log.w(TAG, e, "Interrupted takeNextItem");
    }
    return false;
  }

  private void addItem(PlayableItem item) {
    final OnDemandUpdateItem wrapper = new OnDemandUpdateItem(item);
    totalUsedMemory += wrapper.getUsedMemory();
    history.add(0, wrapper);
    for (int index = history.size() - 1; totalUsedMemory > MAX_USED_MEMORY && index != 0; --index) {
      final OnDemandUpdateItem toCleanup = history.get(index);
      final long used = toCleanup.getUsedMemory();
      if (used != 0) {
        totalUsedMemory -= used;
        toCleanup.capture().getModule().release();
      }
    }
  }

  private static java.util.Iterator<VfsFile> creatDirFilesIterator(Uri start) throws Exception {
    final ArrayList<VfsFile> result = new ArrayList<>();
    final VfsObject obj = VfsArchive.resolveForced(start, StubProgressCallback.instance());
    if (obj == null) {
      return result.listIterator();
    } else if (!(obj instanceof VfsFile)) {
      final Object feed = obj.getExtension(VfsExtensions.FEED);
      return feed != null ? (java.util.Iterator<VfsFile>) feed : result.listIterator();
    }
    final VfsObject parent = obj.getParent();
    if (parent == null) {
      result.add((VfsFile)obj);
      return result.listIterator();
    }
    final VfsFile parentFile = parent instanceof VfsFile ? (VfsFile) parent : null;
    final VfsDir parentDir = parent instanceof VfsDir ? (VfsDir) parent : (VfsDir) parentFile.getParent();
    {
      final Object feed = findFeed(parentDir);
      if (feed != null) {
        return new FeedIterator((VfsFile) obj, (java.util.Iterator<VfsFile>) feed);
      }
    }
    parentDir.enumerate(new VfsDir.Visitor() {
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
    final Object extension = parentDir.getExtension(VfsExtensions.COMPARATOR);
    final Comparator<VfsObject> comparator = extension instanceof Comparator<?>
                                                 ? (Comparator<VfsObject>) extension
                                                 : DefaultComparator.instance();
    Collections.sort(result, comparator);
    // Resolved file may be incomparable with dir content due to lack of some properties
    // E.g. track from zxart/Top doesn't have rating info
    // So use plain linear search by uri
    final Uri normalizedUri = obj.getUri();
    final Uri parentFileUri = parentFile != null ? parentFile.getUri() : null;
    for (int idx = 0, lim = result.size(); idx < lim; ++idx) {
      final VfsFile cur = result.get(idx);
      final Uri curUri = cur.getUri();
      if (normalizedUri.equals(curUri) || curUri.equals(parentFileUri)) {
        return result.listIterator(idx);
      }
    }
    return result.listIterator();
  }

  @Nullable
  private static Object findFeed(VfsObject start) {
    for (VfsObject obj = start; obj != null; obj = obj.getParent()) {
      final Object feed = obj.getExtension(VfsExtensions.FEED);
      if (feed != null) {
        return feed;
      }
    }
    return null;
  }

  // Adapter to prepend feed with already known item
  private static class FeedIterator implements java.util.Iterator<VfsFile> {

    private final java.util.Iterator<VfsFile> delegate;
    @Nullable
    private VfsFile first;

    FeedIterator(VfsFile first, java.util.Iterator<VfsFile> delegate) {
      this.first = first;
      this.delegate = delegate;
    }

    @Override
    public boolean hasNext() {
      return first != null || delegate.hasNext();
    }

    @Override
    public VfsFile next() {
      if (first != null) {
        final VfsFile res = first;
        first = null;
        return res;
      } else {
        return delegate.next();
      }
    }
  }
}
