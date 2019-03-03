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
import android.support.v4.media.session.MediaButtonReceiver;
import android.support.v4.media.session.MediaControllerCompat;
import android.support.v4.media.session.MediaSessionCompat;
import android.support.v4.media.session.PlaybackStateCompat;
import android.view.View;
import android.widget.RemoteViews;
import app.zxtune.MainActivity;
import app.zxtune.R;

public class WidgetHandler extends AppWidgetProvider {

  public static void connect(Context ctx, MediaSessionCompat session) {
    final MediaControllerCompat controller = session.getController();
    controller.registerCallback(new WidgetNotification(ctx));
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
    widgetView.setOnClickPendingIntent(R.id.widget_art, MainActivity.createPendingIntent(context));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_prev, MediaButtonReceiver.buildMediaButtonPendingIntent(context,
        PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_play, MediaButtonReceiver.buildMediaButtonPendingIntent(context, PlaybackStateCompat.ACTION_PLAY));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_pause,
        MediaButtonReceiver.buildMediaButtonPendingIntent(context, PlaybackStateCompat.ACTION_STOP));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_next,
        MediaButtonReceiver.buildMediaButtonPendingIntent(context, PlaybackStateCompat.ACTION_SKIP_TO_NEXT));

    widgetView.setViewVisibility(R.id.widget_ctrl_pause, View.GONE);

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
      views.setViewVisibility(R.id.widget_ctrl_play, isPlaying ? View.GONE : View.VISIBLE);
      views.setViewVisibility(R.id.widget_ctrl_pause, isPlaying ? View.VISIBLE : View.GONE);

      updateView();
    }

    @Override
    public void onMetadataChanged(MediaMetadataCompat metadata) {
      final MediaDescriptionCompat description = metadata.getDescription();
      views.setTextViewText(R.id.widget_title, description.getTitle());
      views.setTextViewText(R.id.widget_subtitle, description.getSubtitle());
      views.setImageViewBitmap(R.id.widget_art, description.getIconBitmap());

      updateView();
    }

    private void updateView() {
      update(ctx, views);
    }
  }
}
