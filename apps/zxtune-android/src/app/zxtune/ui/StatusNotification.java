/*
 * @file
 * @brief Status notification support
 * @version $Id:$
 * @author (C) Vitamin/CAIG
 */

package app.zxtune.ui;

import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.support.v4.app.NotificationCompat;
import app.zxtune.R;
import app.zxtune.playback.Control;
import app.zxtune.playback.Item;
import app.zxtune.playback.Callback;
import app.zxtune.playback.Status;

public class StatusNotification implements Callback {

  private final Service service;
  private final NotificationManager manager;
  private final NotificationCompat.Builder builder;
  private final static int notificationId = R.drawable.ic_stat_notify_play;

  public StatusNotification(Service service, Intent intent) {
    this.service = service;
    this.manager = (NotificationManager) service.getSystemService(Context.NOTIFICATION_SERVICE);
    this.builder = new NotificationCompat.Builder(service);
    builder.setOngoing(true);
    builder.setContentIntent(PendingIntent.getActivity(service, 0, intent, 0));
  }
  
  @Override
  public void onControlChanged(Control control) {
  }

  @Override
  public void onItemChanged(Item item) {
    String title = item.getTitle();
    final String author = item.getAuthor();
    String ticker;
    if (0 == title.length() + author.length()) {
      ticker = title = item.getDataId().toString();
    } else if (0 == author.length()) {
      ticker = title;
    } else if (0 == title.length()) {
      ticker = author;
    } else {
      ticker = author + " - " + title;
    }
    builder.setTicker(ticker).setContentTitle(title).setContentText(author);
    showNotification();
  }

  @Override
  public void onStatusChanged(boolean isPlaying) {
    if (isPlaying) {
      builder.setSmallIcon(R.drawable.ic_stat_notify_play);
      service.startForeground(notificationId, showNotification());
    } else {
      manager.cancel(notificationId);
      service.stopForeground(true);
    }
  }

  private Notification showNotification() {
    final Notification notification = builder.build();
    manager.notify(notificationId, notification);
    return notification;
  }
}
