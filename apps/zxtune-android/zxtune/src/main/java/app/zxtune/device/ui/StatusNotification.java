/**
 * @file
 * @brief Status notification support
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.device.ui;

import android.app.Service;
import android.support.v4.app.NotificationCompat;
import android.support.v4.app.NotificationCompat.Action;
import android.support.v4.content.ContextCompat;
import android.support.v4.media.MediaDescriptionCompat;
import android.support.v4.media.MediaMetadataCompat;
import android.support.v4.media.app.NotificationCompat.MediaStyle;
import android.support.v4.media.session.MediaButtonReceiver;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.MediaSessionCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import app.zxtune.MainService;
import app.zxtune.R;

public class StatusNotification extends MediaControllerCompat.Callback {

  private static final int ACTION_PREV = 0;
  private static final int ACTION_PLAY_PAUSE = 1;
  private static final int ACTION_NEXT = 2;

  private final Service service;
  private final Notifications.Controller notification;
  private boolean isPlaying = false;

  public static void connect(Service service, MediaSessionCompat session) {
    final MediaControllerCompat controller = session.getController();
    controller.registerCallback(new StatusNotification(service, session));
  }

  private StatusNotification(Service service, MediaSessionCompat session) {
    this.service = service;
    this.notification = Notifications.createForService(service, R.drawable.ic_stat_notify_play);
    notification.getBuilder()
        .setContentIntent(session.getController().getSessionActivity())
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
    // Hack for Android O requirement to call Service.startForeground after Context.startForegroundService.
    // Service.startForeground may be called from android.support.v4.media.session.MediaButtonReceiver, but
    // no playback may happens.
    startForeground();
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

  private void startForeground() {
    ContextCompat.startForegroundService(service, MainService.createIntent(service, null));
    service.startForeground(notification.getId(), notification.show());
  }

  @Override
  public void onPlaybackStateChanged(PlaybackStateCompat state) {
    if (state == null) {
      return;
    }
    isPlaying = state.getState() == PlaybackStateCompat.STATE_PLAYING;
    notification.getBuilder().mActions.set(ACTION_PLAY_PAUSE, isPlaying ? createPauseAction() : createPlayAction());
    if (isPlaying) {
      startForeground();
    } else {
      notification.show();
      service.stopForeground(false);
    }
  }

  @Override
  public void onMetadataChanged(MediaMetadataCompat metadata) {
    final MediaDescriptionCompat description = metadata.getDescription();
    notification.getBuilder()
        .setContentTitle(description.getTitle())
        .setContentText(description.getSubtitle())
        .setLargeIcon(description.getIconBitmap());
    if (isPlaying) {
      notification.show();
    }
  }
}
