/**
 * @file
 * @brief Implementation of Iterator based on Vfs objects
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.playback;

import android.content.Context;
import android.net.Uri;
import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.playback.stubs.PlayableItemStub;

import java.util.ArrayList;
import java.util.concurrent.LinkedBlockingQueue;

public class FileIterator implements Iterator {

  private static final String TAG = FileIterator.class.getName();

  private static final int MAX_VISITED = 10;

  private final LinkedBlockingQueue<PlayableItem> itemsQueue;
  private final ArrayList<PlayableItem> history;
  private final Object scanHandle;
  private int historyDepth;
  private Exception lastError;

  public static FileIterator create(Context ctx, Uri[] uris) throws Exception {
    return new FileIterator(ctx, uris);
  }

  private FileIterator(Context context, Uri[] uris) throws Exception {
    this.itemsQueue = new LinkedBlockingQueue<>(2);
    this.history = new ArrayList<>(MAX_VISITED + 1);
    this.scanHandle = startAsyncScanning(uris);
    this.historyDepth = 0;
    if (!takeNextItem()) {
      if (lastError != null) {
        throw lastError;
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

  private Object startAsyncScanning(final Uri[] uris) {
    return AsyncScanner.scan(uris, new AsyncScanner.Callback() {

      private final int[] counter = {0, 0};

      @Override
      public Reply onUriProcessing(Uri uri) {
        final int uriIdx = counter[0]++;
        final int itemsCount = counter[1];
        return itemsCount == 0 && uriIdx > 0
                   ? Reply.STOP
                   : Reply.CONTINUE;
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

      @Override
      public void onFinish() {
        try {
          itemsQueue.put(PlayableItemStub.instance());
        } catch (InterruptedException e) {
          Log.w(TAG, e, "Interrupted Callback.onFinish");
        }
      }

      @Override
      public void onError(Exception e) {
        lastError = e;
      }
    });
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
}
