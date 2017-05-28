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
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.Build;
import android.os.Handler;
import android.widget.RemoteViews;

import app.zxtune.Log;
import app.zxtune.MainActivity;
import app.zxtune.MainService;
import app.zxtune.R;
import app.zxtune.Util;
import app.zxtune.playback.CallbackStub;
import app.zxtune.playback.Item;

public class StatusNotification extends CallbackStub {
  
  public enum Type {
    DEFAULT,
    WITH_CONTROLS
  }

  private static final String TAG = StatusNotification.class.getName();
  
  //http://stackoverflow.com/questions/12586938/clickable-custom-view-in-notification-on-android-2-3-or-lower
  public static final boolean BUTTONS_SUPPORTED = Build.VERSION.SDK_INT >= Build.VERSION_CODES.HONEYCOMB;
  
  private final Type type;
  private final Handler scheduler;
  private final Runnable delayedHide;
  private final Service service;
  private final NotificationManager manager;
  private final Notification.Builder builder;
  private final RemoteViews content;
  private static final int notificationId = R.drawable.ic_stat_notify_play;
  private static final int NOTIFICATION_DELAY = 200;
  
  public StatusNotification(Service service, Type type) {
    this.type = type;
    this.scheduler = new Handler();
    this.delayedHide = new DelayedHideCallback();
    this.service = service;
    this.manager = (NotificationManager) service.getSystemService(Context.NOTIFICATION_SERVICE);
    this.builder = new Notification.Builder(service);
    this.content = new RemoteViews(service.getPackageName(), R.layout.notification);
    builder.setOngoing(true);
    builder.setContentIntent(createActivateIntent());
    content.setOnClickPendingIntent(R.id.notification_ctrl_stop, createStopIntent());
    content.setOnClickPendingIntent(R.id.notification_ctrl_next, createNavigateNextIntent());
  }
  
  private PendingIntent createActivateIntent() {
    final Intent intent = new Intent(service, MainActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    return PendingIntent.getActivity(service, 0, intent, 0);
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
      if (BUTTONS_SUPPORTED && Type.WITH_CONTROLS.equals(type)) {
        content.setTextViewText(R.id.notification_title, title);
        content.setTextViewText(R.id.notification_author, author);
        builder.setContent(content);
      } else {
        builder.setContentTitle(title).setContentText(author);
      }
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
    final Notification notification = builder.getNotification();
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
