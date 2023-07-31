/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.device.ui;

import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.ComponentName;
import android.content.Context;
import android.support.v4.media.MediaDescriptionCompat;
import android.support.v4.media.MediaMetadataCompat;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.MediaSessionCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import android.widget.RemoteViews;

import androidx.media.session.MediaButtonReceiver;

import app.zxtune.MainActivity;
import app.zxtune.R;
import app.zxtune.Releaseable;

public class WidgetHandler extends AppWidgetProvider {

  public static Releaseable connect(Context ctx, MediaSessionCompat session) {
    final MediaControllerCompat controller = session.getController();
    final MediaControllerCompat.Callback cb = new WidgetNotification(ctx);
    controller.registerCallback(cb);
    return () -> controller.unregisterCallback(cb);
  }

  @Override
  public void onUpdate(Context context, AppWidgetManager appWidgetManager,
                       int[] appWidgetIds) {
    super.onUpdate(context, appWidgetManager, appWidgetIds);

    update(context, createView(context));
  }

  private static void update(Context context, RemoteViews widgetView) {
    final AppWidgetManager mgr = AppWidgetManager.getInstance(context);
    final int[] widgets = mgr.getAppWidgetIds(new ComponentName(context, WidgetHandler.class));
    if (widgets.length != 0) {
      mgr.updateAppWidget(widgets, widgetView);
    }
  }

  private static RemoteViews createView(Context context) {
    final RemoteViews widgetView = new RemoteViews(context.getPackageName(), R.layout.widget);
    widgetView.setOnClickPendingIntent(R.id.widget, MainActivity.createPendingIntent(context));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_prev, MediaButtonReceiver.buildMediaButtonPendingIntent(context,
        PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_play_pause,
        MediaButtonReceiver.buildMediaButtonPendingIntent(context, PlaybackStateCompat.ACTION_PLAY_PAUSE));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_next,
        MediaButtonReceiver.buildMediaButtonPendingIntent(context, PlaybackStateCompat.ACTION_SKIP_TO_NEXT));

    return widgetView;
  }

  private static class WidgetNotification extends MediaControllerCompat.Callback {

    private final Context ctx;
    private final RemoteViews views;

    WidgetNotification(Context ctx) {
      this.ctx = ctx;
      this.views = createView(ctx);
    }

    @Override
    public void onPlaybackStateChanged(PlaybackStateCompat state) {
      if (state == null) {
        return;
      }
      final boolean isPlaying = state.getState() == PlaybackStateCompat.STATE_PLAYING;
      views.setImageViewResource(R.id.widget_ctrl_play_pause, isPlaying ? R.drawable.ic_pause : R.drawable.ic_play);
      updateView();
    }

    @Override
    public void onMetadataChanged(MediaMetadataCompat metadata) {
      final MediaDescriptionCompat description = metadata.getDescription();
      views.setTextViewText(R.id.widget_title, description.getTitle());

      updateView();
    }

    private void updateView() {
      update(ctx, views);
    }
  }
}
