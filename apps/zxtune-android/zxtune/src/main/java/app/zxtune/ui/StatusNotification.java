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
import android.app.PendingIntent;
import android.app.Service;
import android.content.Intent;
import android.os.Handler;
import android.support.v4.app.NotificationCompat.Action;
import android.support.v4.app.NotificationManagerCompat;
import android.support.v7.app.NotificationCompat;

import app.zxtune.Log;
import app.zxtune.MainActivity;
import app.zxtune.MainService;
import app.zxtune.R;
import app.zxtune.Util;
import app.zxtune.playback.CallbackStub;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;

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
      .setLargeIcon(null)
      .setWhen(0)
      .setContentIntent(createActivateIntent())
      .addAction(R.drawable.ic_prev, "", createServiceIntent(MainService.ACTION_PREV))
      .addAction(createPauseAction())
      .addAction(R.drawable.ic_stop, "", createServiceIntent(MainService.ACTION_STOP))
      .addAction(R.drawable.ic_next, "", createServiceIntent(MainService.ACTION_NEXT))
      .setStyle(new NotificationCompat.MediaStyle()
                      .setShowActionsInCompactView(0, 1, 2, 3)
              //Takes way too much place on 4.4.2
              //.setCancelButtonIntent(createServiceIntent(MainService.ACTION_STOP))
              //.setShowCancelButton(true)
      )
    ;
  }
  
  private PendingIntent createActivateIntent() {
    final Intent intent = new Intent(service, MainActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    return PendingIntent.getActivity(service, 0, intent, 0);
  }
  
  private Action createPauseAction() {
    return new NotificationCompat.Action(R.drawable.ic_pause, "", createServiceIntent(MainService.ACTION_PAUSE));
  }

  private Action createPlayAction() {
    return new NotificationCompat.Action(R.drawable.ic_play, "", createServiceIntent(MainService.ACTION_PLAY));
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
      builder
//        .setTicker(ticker)
        .setContentTitle(title)
        .setContentText(author);
    } catch (Exception e) {
      Log.w(TAG, e, "onIntemChanged()");
    }
  }

  @Override
  public void onStateChanged(PlaybackControl.State state) {
    final boolean isPlaying = state != PlaybackControl.State.STOPPED;
    final boolean isPaused = state == PlaybackControl.State.PAUSED;
    if (isPlaying) {
      scheduler.removeCallbacks(delayedHide);
      builder.mActions.set(1, isPaused ? createPlayAction() : createPauseAction());
      showNotification();
    } else {
      scheduler.postDelayed(delayedHide, NOTIFICATION_DELAY);
    }
  }
  
  private void showNotification() {
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
