/**
 * @file
 * @brief Status notification support
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.app.PendingIntent;
import android.app.Service;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.os.Handler;
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationCompat.Action;
import android.support.v4.content.res.ResourcesCompat;
import android.support.v4.media.app.NotificationCompat.MediaStyle;
import android.support.v4.media.session.MediaSessionCompat;
import app.zxtune.Identifier;
import app.zxtune.Log;
import app.zxtune.MainActivity;
import app.zxtune.MainService;
import app.zxtune.R;
import app.zxtune.Util;
import app.zxtune.device.ui.Notifications;
import app.zxtune.fs.Vfs;
import app.zxtune.fs.VfsExtensions;
import app.zxtune.fs.VfsObject;
import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.stubs.CallbackStub;

public class StatusNotification extends CallbackStub {

  private static final String TAG = StatusNotification.class.getName();

  private static final int NOTIFICATION_DELAY = 200;

  private final Handler scheduler;
  private final Runnable delayedHide;
  private final Service service;
  private final Notifications.Controller notification;

  public StatusNotification(Service service, MediaSessionCompat.Token sessionToken) {
    this.scheduler = new Handler();
    this.delayedHide = new DelayedHideCallback();
    this.service = service;
    this.notification = Notifications.createForService(service, R.drawable.ic_stat_notify_play);
    // Little trick for Android O that requires startForeground call in 5 seconds after service start
    // But initialization may take longer...
    showNotification();
    notification.getBuilder()
        .setContentIntent(MainActivity.createPendingIntent(service))
        .addAction(R.drawable.ic_prev, "", createServiceIntent(MainService.ACTION_PREV))
        .addAction(createPauseAction())
        .addAction(R.drawable.ic_next, "", createServiceIntent(MainService.ACTION_NEXT))
        .addAction(R.drawable.ic_stop, "", createServiceIntent(MainService.ACTION_STOP))
        .setStyle(new MediaStyle()
                .setShowActionsInCompactView(0/*prev*/, 1/*play/pause*/, 2/*next*/)
                .setMediaSession(sessionToken)
            //Takes way too much place on 4.4.2
            //.setCancelButtonIntent(createServiceIntent(MainService.ACTION_STOP))
            //.setShowCancelButton(true)
        )
    ;
    service.stopForeground(true);
  }

  private Action createPauseAction() {
    return new NotificationCompat.Action(R.drawable.ic_pause, "", createServiceIntent(MainService.ACTION_PAUSE));
  }

  private Action createPlayAction() {
    return new NotificationCompat.Action(R.drawable.ic_play, "", createServiceIntent(MainService.ACTION_PLAY));
  }

  private PendingIntent createServiceIntent(String action) {
    return MainService.createPendingIntent(service, action);
  }

  @Override
  public void onItemChanged(Item item) {
    try {
      final Identifier dataId = item.getDataId();
      final String filename = dataId.getDisplayFilename();
      String title = item.getTitle();
      final String author = item.getAuthor();
      final String ticker = Util.formatTrackTitle(title, author, filename);
      if (ticker.equals(filename)) {
        title = filename;
      }
      notification.getBuilder()
//        .setTicker(ticker)
          .setContentTitle(title)
          .setContentText(author)
          .setContentText(author)
          .setLargeIcon(getLocationIcon(dataId.getDataLocation()));
    } catch (Exception e) {
      Log.w(TAG, e, "onIntemChanged()");
    }
  }

  @Override
  public void onStateChanged(PlaybackControl.State state) {
    final boolean isPlaying = state != PlaybackControl.State.STOPPED;
    final boolean isPaused = state == PlaybackControl.State.PAUSED;
    if (isPlaying) {
      notification.getBuilder().mActions.set(1, isPaused ? createPlayAction() : createPauseAction());
      showNotification();
    } else {
      hideNotification();
    }
  }

  private void showNotification() {
    scheduler.removeCallbacks(delayedHide);
    service.startForeground(notification.getId(), notification.show());
  }

  private void hideNotification() {
    scheduler.postDelayed(delayedHide, NOTIFICATION_DELAY);
  }

  private class DelayedHideCallback implements Runnable {
    @Override
    public void run() {
      notification.hide();
      service.stopForeground(true);
    }
  }

  private Bitmap getLocationIcon(Uri location) {
    try {
      final Resources resources = service.getResources();
      final int id = getLocationIconResource(location);
      final Drawable drawable = ResourcesCompat.getDrawableForDensity(resources, id, 480/*XXHDPI*/, null);
      if (drawable instanceof BitmapDrawable) {
        return ((BitmapDrawable) drawable).getBitmap();
      } else {
        final Bitmap result = Bitmap.createBitmap(128,128, Bitmap.Config.ARGB_8888);
        final Canvas canvas = new Canvas(result);
        drawable.setBounds(0, 0, canvas.getWidth(), canvas.getHeight());
        drawable.draw(canvas);
        return result;
      }
    } catch (Exception e) {
    }
    return null;
  }

  private int getLocationIconResource(Uri location) {
    try {
      final Uri rootLocation = new Uri.Builder().scheme(location.getScheme()).build();
      final VfsObject root = Vfs.resolve(rootLocation);
      return (Integer) root.getExtension(VfsExtensions.ICON_RESOURCE);
    } catch (Exception e) {
      return R.drawable.ic_launcher;
    }
  }
}
