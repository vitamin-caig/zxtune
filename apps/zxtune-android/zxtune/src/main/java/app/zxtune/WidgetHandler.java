/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune;

import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.ComponentName;
import android.content.Context;
import android.view.View;
import android.widget.RemoteViews;

import app.zxtune.playback.Item;
import app.zxtune.playback.PlaybackControl;
import app.zxtune.playback.stubs.CallbackStub;

public class WidgetHandler extends AppWidgetProvider {

  private static final String TAG = WidgetHandler.class.getName();

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

    widgetView.setOnClickPendingIntent(R.id.widget_openapp, MainActivity.createPendingIntent(context));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_prev, MainService.createPendingIntent(context, MainService.ACTION_PREV));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_play, MainService.createPendingIntent(context, MainService.ACTION_PLAY));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_pause, MainService.createPendingIntent(context, MainService.ACTION_STOP));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_next, MainService.createPendingIntent(context, MainService.ACTION_NEXT));

    widgetView.setViewVisibility(R.id.widget_ctrl_pause, View.GONE);

    return widgetView;
  }
  
  public static class WidgetNotification extends CallbackStub {

    private final Context ctx;
    private final RemoteViews views;

    public WidgetNotification(Context ctx) {
      this.ctx = ctx;
      this.views = createView(ctx);
    }

    @Override
    public void onStateChanged(PlaybackControl.State state) {
      final boolean playing = state == PlaybackControl.State.PLAYING;
      views.setViewVisibility(R.id.widget_ctrl_play, playing ? View.GONE : View.VISIBLE);
      views.setViewVisibility(R.id.widget_ctrl_pause, playing ? View.VISIBLE : View.GONE);

      updateView();
    }

    @Override
    public void onItemChanged(Item item) {

      try {
        final String info = Util.formatTrackTitle(item.getAuthor(), item.getTitle(), item.getDataId().getDisplayFilename());
        views.setTextViewText(R.id.widget_text, info);

        updateView();
      } catch (Exception e) {
        Log.w(TAG, e, "update()");
      }
    }

    private void updateView() {
      update(ctx, views);
    }
  }
}
