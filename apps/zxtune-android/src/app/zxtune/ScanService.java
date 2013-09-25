/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.atomic.AtomicInteger;

import android.app.IntentService;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Handler;
import android.os.Parcelable;
import android.support.v4.app.NotificationCompat;
import android.util.Log;
import android.view.ViewDebug.FlagToString;
import app.zxtune.playback.FileIterator;
import app.zxtune.playback.PlayableItem;
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
  private volatile boolean canceled;

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
      Log.d(TAG, "Canceling");
      canceled = true;
      stopSelf();
    } else {
      super.onStart(intent, startId);
    }
  }
  
  @Override
  public void onDestroy() {
    super.onDestroy();
    try {
      insertThread.interrupt();
      insertThread.join();
    } catch (InterruptedException e) {
      Log.d(TAG, "Interrupted while InsertThread join", e);
    }
  }

  @Override
  protected void onHandleIntent(Intent intent) {
    if (canceled) {
      return;
    } else if (intent.getAction().equals(ACTION_START)) {
      final Parcelable[] paths = intent.getParcelableArrayExtra(EXTRA_PATHS);
      scan(paths);
    }
  }
  
  private void scan(Parcelable[] paths) {
    final Uri[] uris = new Uri[paths.length];
    System.arraycopy(paths, 0, uris, 0, uris.length);

    try {
      scan(uris);
    } catch (Error e) {
      Log.w(TAG, "scan failed", e);
    }
  }
  
  private void scan(Uri[] uris) {
    Log.d(TAG, "scan on " + uris);
    final FileIterator iter = new FileIterator(this, uris);
    do {
      insertThread.enqueue(iter.getItem());
    } while (!canceled && iter.next());
  }
  
  private class InsertItemsThread extends Thread {

    private static final int MAX_QUEUE_SIZE = 100; 
    private final ArrayBlockingQueue<PlayableItem> queue;
    
    InsertItemsThread() {
      super("InsertItemsThread");
      this.queue = new ArrayBlockingQueue<PlayableItem>(MAX_QUEUE_SIZE);
    }
    
    public final void enqueue(PlayableItem item) {
      try {
        queue.put(item);
      } catch (InterruptedException e) {
        Log.d(TAG, "InsertThread: failed to enqueue", e);
        throw new Error(e);
      }
    }
    
    @Override
    public void run() {
      try {
        Log.d(TAG, "InsertThread: started");
        transferSingleValue();
        //start tracking only after first arrive
        tracking.start();
        transferValues();
      } catch (InterruptedException e) {
      } finally {
        Log.d(TAG, "InsertThread: stopping");
        tracking.stop();
        cleanup();
      }
    }
    
    private void transferValues() throws InterruptedException {
      while (!canceled) {
        transferSingleValue();
      }
    }
    
    private void transferSingleValue() throws InterruptedException {
      final PlayableItem item = queue.take();
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
    
    private final static int NOTIFICATION_DELAY = 1000;
    private final static int NOTIFICATION_PERIOD = 5000;
    
    private StatusNotification notification;
    
    public final void start() {
      notification = new StatusNotification();
      timer.postDelayed(this, NOTIFICATION_DELAY);
    }
    
    public final void stop() {
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
    
    private void notifyResolver() {
      getContentResolver().notifyChange(INSERTION_URI, null);
    }
    
    private class StatusNotification {
      
      private final NotificationManager manager;
      private final NotificationCompat.Builder builder;
      private final static int notificationId = R.drawable.ic_launcher;//TODO

      public StatusNotification() {
        this.manager = (NotificationManager) ScanService.this.getSystemService(Context.NOTIFICATION_SERVICE);
        this.builder = new NotificationCompat.Builder(ScanService.this);
        final Intent cancelIntent = new Intent(ScanService.this, ScanService.class);
        cancelIntent.setAction(ACTION_CANCEL);
        builder
          .setContentIntent(PendingIntent.getService(ScanService.this, 0, cancelIntent, PendingIntent.FLAG_UPDATE_CURRENT))
          .setOngoing(true)
          .setProgress(0, 0, true)
          .setSmallIcon(R.drawable.ic_launcher)
          .setContentTitle(getResources().getText(R.string.scanning_title))
          .setContentText(getResources().getText(R.string.scanning_text))
        ;
      }
      
      public final void show() {
        builder.setNumber(addedItems.get());
        final Notification notification = builder.build();
        manager.notify(notificationId, notification);
      }
      
      public final void hide() {
        manager.cancel(notificationId);
      }
    }
  }
}
