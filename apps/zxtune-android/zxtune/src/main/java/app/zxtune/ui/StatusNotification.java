/**
 *
 * @file
 *
 * @brief Status notification support
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.ui;

import android.app.Notification;
import android.support.v7.app.NotificationCompat;
import android.support.v4.app.NotificationManagerCompat;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Handler;

import app.zxtune.Log;
import app.zxtune.MainActivity;
import app.zxtune.MainService;
import app.zxtune.R;
import app.zxtune.Util;
import app.zxtune.playback.CallbackStub;
import app.zxtune.playback.Item;

public class StatusNotification extends CallbackStub {
  
  private static final String TAG = StatusNotification.class.getName();
  
  private final Handler scheduler;
  private final Runnable delayedHide;
  private final Service service;
  private final NotificationManagerCompat manager;
  private final NotificationCompat.Builder builder;
  private static final int notificationId = R.drawable.ic_stat_notify_play;
  private static final int NOTIFICATION_DELAY = 200;
  
  public StatusNotification(Service service) {
    this.scheduler = new Handler();
    this.delayedHide = new DelayedHideCallback();
    this.service = service;
    this.manager = NotificationManagerCompat.from(service);
    this.builder = new NotificationCompat.Builder(service);
    builder
      .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
      .setOngoing(true)
      .setSmallIcon(R.drawable.ic_stat_notify_play)
      .setContentIntent(createActivateIntent())
      .addAction(R.drawable.ic_prev, "", createNavigatePrevIntent())
      .addAction(R.drawable.ic_stop, "", createStopIntent())
      .addAction(R.drawable.ic_next, "", createNavigateNextIntent())
      .setStyle(new NotificationCompat.MediaStyle()
        .setShowActionsInCompactView(0, 1, 2)
        //TODO: enable when pause mode will be available
        //.setCancelButtonIntent(createStopIntent())
        //.setShowCancelButton(true)
      )
    ;
  }
  
  private PendingIntent createActivateIntent() {
    final Intent intent = new Intent(service, MainActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    return PendingIntent.getActivity(service, 0, intent, 0);
  }
  
  private PendingIntent createNavigatePrevIntent() {
    return createServiceIntent(MainService.ACTION_PREV);
  }

  private PendingIntent createStopIntent() {
    return createServiceIntent(MainService.ACTION_PAUSE);
  }

  private PendingIntent createNavigateNextIntent() {
    return createServiceIntent(MainService.ACTION_NEXT);
  }
  
  private PendingIntent createServiceIntent(String action) {
    final Intent intent = new Intent(service, service.getClass());
    intent.setAction(action);
    return PendingIntent.getService(service, 0, intent, 0);
  }
  
  @Override
  public void onItemChanged(Item item) {
    try {
      final String filename = item.getDataId().getDisplayFilename();
      String title = item.getTitle();
      final String author = item.getAuthor();
      final String ticker = Util.formatTrackTitle(title, author, filename);
      if (ticker.equals(filename)) {
        title = filename;
      }
      builder.setTicker(ticker);
      builder.setContentTitle(title).setContentText(author);
    } catch (Exception e) {
      Log.w(TAG, e, "onIntemChanged()");
    }
  }

  @Override
  public void onStatusChanged(boolean isPlaying) {
    if (isPlaying) {
      scheduler.removeCallbacks(delayedHide);
      showNotification();
    } else {
      scheduler.postDelayed(delayedHide, NOTIFICATION_DELAY);
    }
  }
  
  private void showNotification() {
    builder.setSmallIcon(R.drawable.ic_stat_notify_play);
    service.startForeground(notificationId, makeNotification());
  }
  
  private void hideNotification() {
    manager.cancel(notificationId);
    service.stopForeground(true);
  }

  private Notification makeNotification() {
    final Notification notification = builder.build();
    manager.notify(notificationId, notification);
    return notification;
  }
  
  private class DelayedHideCallback implements Runnable {
    @Override
    public void run() {
      hideNotification();
    }
  }
}
