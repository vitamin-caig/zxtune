package app.zxtune.device.ui;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.content.Context;
import android.os.Build;
import androidx.annotation.DrawableRes;
import androidx.annotation.NonNull;
import androidx.annotation.RequiresApi;
import androidx.annotation.StringRes;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import app.zxtune.R;

public final class Notifications {

  private static final String SERVICES_CHANNEL_ID = "services";
  private static final String EVENTS_CHANNEL_ID = "events";

  public static final class Controller {

    private final int id;
    private final NotificationManagerCompat manager;
    private final NotificationCompat.Builder builder;

    Controller(Context ctx, int icon) {
      this.id = icon;
      this.manager = NotificationManagerCompat.from(ctx);
      this.builder = new NotificationCompat.Builder(ctx, SERVICES_CHANNEL_ID);
      builder
          .setSmallIcon(icon)
          .setShowWhen(false);
    }

    public NotificationCompat.Builder getBuilder() {
      return builder;
    }

    public int getId() {
      return id;
    }

    public Notification show() {
      final Notification result = builder.build();
      manager.notify(id, result);
      return result;
    }

    public void hide() {
      manager.cancel(id);
    }
  }

  public static Controller createForService(@NonNull Context ctx, @DrawableRes int icon) {
    return new Controller(ctx, icon);
  }

  public static void sendEvent(@NonNull Context ctx, @DrawableRes int icon, @StringRes int title, @NonNull String text) {
    final NotificationManager manager = (NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
    if (manager != null) {
      final NotificationCompat.Builder builder = new NotificationCompat.Builder(ctx, EVENTS_CHANNEL_ID);
      builder.setOngoing(false);
      builder.setSmallIcon(icon);
      builder.setContentTitle(ctx.getString(title));
      builder.setContentText(text);
      manager.notify(text.hashCode(), builder.build());
    }
  }

  public static void setup(@NonNull Context ctx) {
    if (Build.VERSION.SDK_INT >= 26) {
      final NotificationManager manager = (NotificationManager) ctx.getSystemService(Context.NOTIFICATION_SERVICE);
      if (manager != null) {
        manager.createNotificationChannel(createServicesChannel(ctx));
        manager.createNotificationChannel(createEventsChannel(ctx));
      }
    }
  }

  @RequiresApi(26)
  private static NotificationChannel createServicesChannel(Context ctx) {
    final String name = ctx.getString(R.string.notification_channel_name_services);
    final NotificationChannel res = new NotificationChannel(SERVICES_CHANNEL_ID, name, NotificationManager.IMPORTANCE_LOW);
    res.setShowBadge(false);
    res.setLockscreenVisibility(Notification.VISIBILITY_PUBLIC);
    return res;
  }

  @RequiresApi(26)
  private static NotificationChannel createEventsChannel(Context ctx) {
    final String name = ctx.getString(R.string.notification_channel_name_events);
    return new NotificationChannel(EVENTS_CHANNEL_ID, name, NotificationManager.IMPORTANCE_LOW);
  }
}
