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
import app.zxtune.playback.Callback;
import app.zxtune.playback.Item;

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
  public void onItemChanged(Item item) {
    String title = item.getTitle();
    final String author = item.getAuthor();
    final boolean noTitle = 0 == title.length();
    final boolean noAuthor = 0 == author.length();
    final StringBuilder ticker = new StringBuilder();
    if (noTitle && noAuthor) {
      ticker.append(title = item.getDataId().getLastPathSegment());
    } else {
      ticker.append(title);
      if (!noTitle && !noAuthor) {
        ticker.append(" - ");
      }
      ticker.append(author);
    }
    builder.setTicker(ticker.toString()).setContentTitle(title).setContentText(author);
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
