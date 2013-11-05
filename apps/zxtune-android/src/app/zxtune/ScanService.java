/**
 *
 * @file
 *
 * @brief Scanning service
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune;

import java.io.IOException;
import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Handler;
import android.os.Parcelable;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import app.zxtune.playback.Iterator;
import app.zxtune.playback.IteratorFactory;
import app.zxtune.playback.PlayableItem;
import app.zxtune.playback.PlayableItemStub;
import app.zxtune.playlist.Item;
import app.zxtune.playlist.Query;

public class ScanService extends IntentService {

  private static final String TAG = ScanService.class.getName();
  private static final Uri INSERTION_URI = Query.unparse(null);

  public static final String ACTION_START = TAG + ".add";
  public static final String ACTION_CANCEL = TAG + ".cancel";
  public static final String EXTRA_PATHS = "paths";

  private final Handler timer;
  private final NotifyTask tracking;
  private final InsertItemsThread insertThread;
  private final AtomicInteger addedItems;
  
  /**
   * InsertThread is executed for onCreate..onDestroy interval 
   */

  public ScanService() {
    super(ScanService.class.getName());
    this.timer = new Handler();
    this.tracking = new NotifyTask();
    this.insertThread = new InsertItemsThread();
    this.addedItems = new AtomicInteger();
    setIntentRedelivery(false);
  }

  @Override
  public void onCreate() {
    super.onCreate();
    insertThread.start();
  }

  @Override
  public void onStart(Intent intent, int startId) {
    if (intent.getAction().equals(ACTION_CANCEL)) {
      insertThread.cancel();
      stopSelf();
    } else {
      super.onStart(intent, startId);
    }
  }

  @Override
  public void onDestroy() {
    super.onDestroy();
    insertThread.flush();
  }

  @Override
  protected void onHandleIntent(Intent intent) {
    if (intent.getAction().equals(ACTION_START)) {
      final Parcelable[] paths = intent.getParcelableArrayExtra(EXTRA_PATHS);
      scan(paths);
    }
  }

  private void scan(Parcelable[] paths) {
    final Uri[] uris = new Uri[paths.length];
    System.arraycopy(paths, 0, uris, 0, uris.length);

    scan(uris);
  }

  private void scan(Uri[] uris) {
    for (Uri uri : uris) {
      Log.d(TAG, "scan on " + uri);
    }
    try {
      final Iterator iter = IteratorFactory.createIterator(this, uris);
      do {
        final PlayableItem item = iter.getItem();
        try {
          insertThread.enqueue(item);
        } catch (InterruptedException e) {
          Log.d(TAG, "scan interrupted");
          item.release();
          break;
        }
      } while (insertThread.isActive() && iter.next());
    } catch (IOException e) {
      Log.d(TAG, "Scan canceled", e);
      insertThread.cancel();
    }
  }

  private class InsertItemsThread extends Thread {

    private static final int MAX_QUEUE_SIZE = 100;
    private final ArrayBlockingQueue<PlayableItem> queue;
    private final AtomicBoolean active;

    InsertItemsThread() {
      super(TAG + ".InsertItemsThread");
      this.queue = new ArrayBlockingQueue<PlayableItem>(MAX_QUEUE_SIZE);
      this.active = new AtomicBoolean();
    }

    final void enqueue(PlayableItem item) throws InterruptedException {
      if (isActive()) {
        queue.put(item);
      } else {
        item.release();
      }
    }
    
    @Override
    public void start() {
      active.set(true);
      super.start();
    }
    
    final boolean isActive() {
      return active.get();
    }
    
    final void cancel() {
      Log.d(getName(), "cancel()");
      active.set(false);
      interrupt();
    }
    
    final void flush() {
      try {
        Log.d(getName(), "waitForFinish()");
        enqueue(PlayableItemStub.instance());
        active.set(false);
        join();
      } catch (InterruptedException e) {
        Log.d(getName(), "failed to wait for finish", e);
      }
    }
    
    @Override
    public void run() {
      try {
        Log.d(getName(), "started");
        if (transferSingleItem()) {
          //start tracking only after first arrive
          performTransfer();
        }
      } catch (InterruptedException e) {
        Log.d(getName(), "interrupted");
      } finally {
        Log.d(getName(), "stopping");
        cleanup();
      }
    }
    
    private void performTransfer() throws InterruptedException {
      tracking.start();
      try {
        transferItems();
      } finally {
        tracking.stop();
      }
    }

    private void transferItems() throws InterruptedException {
      while (transferSingleItem()) {}
    }
    
    private boolean transferSingleItem() throws InterruptedException {
      final PlayableItem item = queue.take();
      if (item == PlayableItemStub.instance()) {
        return false;
      }
      insertItem(item);
      return true;
    }

    private void insertItem(PlayableItem item) {
      try {
        final Item listItem = new Item(item.getDataId(), item.getModule());
        getContentResolver().insert(INSERTION_URI, listItem.toContentValues());
        addedItems.incrementAndGet();
      } finally {
        item.release();
      }
    }

    private void cleanup() {
      while (!queue.isEmpty()) {
        queue.remove().release();
      }
    }
  }

  private class NotifyTask implements Runnable {

    private final static int NOTIFICATION_DELAY = 100;
    private final static int NOTIFICATION_PERIOD = 2000;

    private WakeLock wakeLock;
    private StatusNotification notification;
    
    final void start() {
      notification = new StatusNotification();
      timer.postDelayed(this, NOTIFICATION_DELAY);
      getWakelock().acquire();
    }

    final void stop() {
      getWakelock().release();
      timer.removeCallbacks(this);
      notifyResolver();
      notification.hide();
      notification = null;
    }

    @Override
    public void run() {
      Log.d(TAG, "Notify about changes");
      timer.postDelayed(this, NOTIFICATION_PERIOD);
      notifyResolver();
      notification.show();
    }
    
    private WakeLock getWakelock() {
      if (wakeLock == null) {
        final PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        wakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "ScanService");
      }
      return wakeLock;
    }

    private void notifyResolver() {
      getContentResolver().notifyChange(INSERTION_URI, null);
    }

    private class StatusNotification {

      private final static int notificationId = R.drawable.ic_stat_notify_scan;
      private final NotificationManager manager;
      private final NotificationCompat.Builder builder;
      private final CharSequence titlePrefix;

      StatusNotification() {
        this.manager =
            (NotificationManager) ScanService.this.getSystemService(Context.NOTIFICATION_SERVICE);
        this.builder = new NotificationCompat.Builder(ScanService.this);
        this.titlePrefix = getResources().getText(R.string.scanning_title);
        final Intent cancelIntent = new Intent(ScanService.this, ScanService.class);
        cancelIntent.setAction(ACTION_CANCEL);
        builder
            .setContentIntent(
                PendingIntent.getService(ScanService.this, 0, cancelIntent,
                    PendingIntent.FLAG_UPDATE_CURRENT)).setOngoing(true).setProgress(0, 0, true)
            .setSmallIcon(notificationId)
            .setContentTitle(titlePrefix)
            .setContentText(getResources().getText(R.string.scanning_text));
      }

      final void show() {
        final StringBuilder str = new StringBuilder();
        str.append(titlePrefix);
        str.append(" ");
        final int items = addedItems.get();
        str.append(getResources().getQuantityString(R.plurals.tracks, items, items));
        builder.setContentTitle(str.toString());
        final Notification notification = builder.build();
        manager.notify(notificationId, notification);
      }

      final void hide() {
        manager.cancel(notificationId);
      }
    }
  }
}
