/**
 * @file
 * @brief Status notification support
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.ui;

import android.app.Service;
import android.content.res.Resources;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.drawable.BitmapDrawable;
import android.graphics.drawable.Drawable;
import android.net.Uri;
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationCompat.Action;
import android.support.v4.content.res.ResourcesCompat;
import android.support.v4.media.app.NotificationCompat.MediaStyle;
import android.support.v4.media.session.MediaButtonReceiver;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.MediaSessionCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import app.zxtune.Identifier;
import app.zxtune.Log;
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

  private static final int ACTION_PREV = 0;
  private static final int ACTION_PLAY_PAUSE = 1;
  private static final int ACTION_NEXT = 2;

  private final Service service;
  private final Notifications.Controller notification;
  private PlaybackControl.State lastState;

  public StatusNotification(Service service, MediaSessionCompat session) {
    this.service = service;
    this.notification = Notifications.createForService(service, R.drawable.ic_stat_notify_play);
    // Little trick for Android O that requires startForeground call in 5 seconds after service start
    // But initialization may take longer...
    startForeground();

    final MediaControllerCompat controller = session.getController();

    notification.getBuilder()
        .setContentIntent(controller.getSessionActivity())
        .addAction(R.drawable.ic_prev, "", MediaButtonReceiver.buildMediaButtonPendingIntent(service,
            PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS))
        .addAction(createPauseAction())
        .addAction(R.drawable.ic_next, "", MediaButtonReceiver.buildMediaButtonPendingIntent(service,
            PlaybackStateCompat.ACTION_SKIP_TO_NEXT))
        .setStyle(new MediaStyle()
                      .setShowActionsInCompactView(ACTION_PREV, ACTION_PLAY_PAUSE, ACTION_NEXT)
                      .setMediaSession(session.getSessionToken())
            //Takes way too much place on 4.4.2
            //.setCancelButtonIntent(createServiceIntent(MainService.ACTION_STOP))
            //.setShowCancelButton(true)
        )
    ;
    service.stopForeground(true);
  }

  private Action createPauseAction() {
    return new NotificationCompat.Action(R.drawable.ic_pause, "",
        MediaButtonReceiver.buildMediaButtonPendingIntent(service, PlaybackStateCompat.ACTION_STOP));
  }

  private Action createPlayAction() {
    return new NotificationCompat.Action(R.drawable.ic_play, "",
        MediaButtonReceiver.buildMediaButtonPendingIntent(service, PlaybackStateCompat.ACTION_PLAY));
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
      if (PlaybackControl.State.PLAYING == lastState) {
        notification.show();
      }
    } catch (Exception e) {
      Log.w(TAG, e, "onIntemChanged()");
    }
  }

  @Override
  public void onStateChanged(PlaybackControl.State state) {
    final boolean isPlaying = state == PlaybackControl.State.PLAYING;
    notification.getBuilder().mActions.set(ACTION_PLAY_PAUSE, isPlaying ? createPauseAction() : createPlayAction());
    if (isPlaying) {
      startForeground();
    } else if (state != PlaybackControl.State.PLAYING) {
      notification.show();
      service.stopForeground(false);
    }
    lastState = state;
  }

  private void startForeground() {
    service.startForeground(notification.getId(), notification.show());
  }

  private Bitmap getLocationIcon(Uri location) {
    try {
      final Resources resources = service.getResources();
      final int id = getLocationIconResource(location);
      final Drawable drawable = ResourcesCompat.getDrawableForDensity(resources, id, 480/*XXHDPI*/, null);
      if (drawable instanceof BitmapDrawable) {
        return ((BitmapDrawable) drawable).getBitmap();
      } else {
        final Bitmap result = Bitmap.createBitmap(128, 128, Bitmap.Config.ARGB_8888);
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
