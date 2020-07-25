/**
 * @file
 * @brief Status notification support
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.device.ui;

import android.app.Service;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationCompat.Action;
import androidx.core.content.ContextCompat;

import android.os.Build;
import android.support.v4.media.MediaDescriptionCompat;
import android.support.v4.media.MediaMetadataCompat;
import androidx.media.app.NotificationCompat.MediaStyle;
import androidx.media.session.MediaButtonReceiver;
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
  private boolean wasForeground = false;

  public static void connect(Service service, MediaSessionCompat session) {
    final MediaControllerCompat controller = session.getController();
    controller.registerCallback(new StatusNotification(service, session));
  }

  private StatusNotification(Service service, MediaSessionCompat session) {
    this.service = service;
    this.notification = Notifications.createForService(service, R.drawable.ic_stat_notify_play);
    notification.getBuilder()
        .setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
        .setContentIntent(session.getController().getSessionActivity())
        .addAction(R.drawable.ic_prev, "", MediaButtonReceiver.buildMediaButtonPendingIntent(service,
            PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS))
        .addAction(createPauseAction())
        .addAction(R.drawable.ic_next, "", MediaButtonReceiver.buildMediaButtonPendingIntent(service,
            PlaybackStateCompat.ACTION_SKIP_TO_NEXT));
    if (!needFixForHuawei()) {
        notification.getBuilder().setStyle(new MediaStyle()
          .setShowActionsInCompactView(ACTION_PREV, ACTION_PLAY_PAUSE, ACTION_NEXT)
          .setMediaSession(session.getSessionToken())
      );
    }
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

  private static boolean needFixForHuawei() {
    return Build.MANUFACTURER.toLowerCase().contains("huawei") && Build.VERSION.SDK_INT < Build.VERSION_CODES.M;
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
      wasForeground = true;
    } else if (wasForeground) {
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
