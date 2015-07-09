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

import android.app.PendingIntent;
import android.appwidget.AppWidgetManager;
import android.appwidget.AppWidgetProvider;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.widget.RemoteViews;
import app.zxtune.playback.CallbackStub;
import app.zxtune.playback.Item;
import app.zxtune.playback.ItemStub;

public class WidgetHandler extends AppWidgetProvider {

  @Override
  public void onUpdate(Context context, AppWidgetManager appWidgetManager,
      int[] appWidgetIds) {
    super.onUpdate(context, appWidgetManager, appWidgetIds);

    update(context, ItemStub.instance());
  }
  
  public static void update(Context context, Item nowPlaying) {
    final AppWidgetManager mgr = AppWidgetManager.getInstance(context);
    final int[] widgets = mgr.getAppWidgetIds(new ComponentName(context, WidgetHandler.class));
    if (widgets.length == 0) {
      return;
    }

    final RemoteViews widgetView = new RemoteViews(context.getPackageName(), R.layout.widget);
    
    widgetView.setOnClickPendingIntent(R.id.widget_openapp, createApplicationIntent(context));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_prev, createServiceIntent(context, MainService.ACTION_PREV));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_play, createServiceIntent(context, MainService.ACTION_PLAY));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_pause, createServiceIntent(context, MainService.ACTION_PAUSE));
    widgetView.setOnClickPendingIntent(R.id.widget_ctrl_next, createServiceIntent(context, MainService.ACTION_NEXT));
    
    final String info = Util.formatTrackTitle(nowPlaying.getAuthor(), nowPlaying.getTitle(), nowPlaying.getDataId().getDisplayFilename());
    widgetView.setTextViewText(R.id.widget_text, info);
    
    mgr.updateAppWidget(widgets, widgetView);
  }
  
  private static PendingIntent createApplicationIntent(Context context) {
    final Intent intent = new Intent(context, MainActivity.class);
    intent.setFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP);
    return PendingIntent.getActivity(context, 0, intent, 0);
  }
  
  private static PendingIntent createServiceIntent(Context context, String action) {
    final Intent intent = new Intent(context, MainService.class);
    intent.setAction(action);
    return PendingIntent.getService(context, 0, intent, 0);
  }
  
  public static class WidgetNotification extends CallbackStub {

    @Override
    public void onItemChanged(Item item) {
      update(MainApplication.getInstance(), item);
    }
  }
}
