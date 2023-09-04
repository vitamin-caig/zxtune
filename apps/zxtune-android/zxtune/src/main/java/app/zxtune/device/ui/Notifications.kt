package app.zxtune.device.ui

import android.annotation.SuppressLint
import android.app.Notification
import android.app.NotificationChannel
import android.app.NotificationManager
import android.content.Context
import android.os.Build
import android.widget.Toast
import androidx.annotation.DrawableRes
import androidx.annotation.RequiresApi
import androidx.annotation.StringRes
import androidx.core.app.NotificationCompat
import androidx.core.app.NotificationManagerCompat
import androidx.core.content.getSystemService
import app.zxtune.R

object Notifications {
    private const val SERVICES_CHANNEL_ID = "services"
    private const val EVENTS_CHANNEL_ID = "events"

    @JvmStatic
    fun createForService(ctx: Context, @DrawableRes icon: Int) = Controller(ctx, icon)

    fun sendEvent(ctx: Context, @DrawableRes icon: Int, @StringRes title: Int, text: String) {
        val manager = ctx.getSystemService<NotificationManager>()
        if (manager != null && canSendNotifications(manager)) {
            NotificationCompat.Builder(ctx, EVENTS_CHANNEL_ID).setOngoing(false).setSmallIcon(icon)
                .setContentTitle(ctx.getString(title)).setContentText(text).run {
                    manager.notify(text.hashCode(), build())
                }
        } else {
            Toast.makeText(ctx, "${ctx.getString(title)}\n$text", Toast.LENGTH_LONG).show()
        }
    }

    private fun canSendNotifications(manager: NotificationManager) =
        Build.VERSION.SDK_INT < 24 || manager.areNotificationsEnabled()

    @JvmStatic
    fun setup(ctx: Context) {
        if (Build.VERSION.SDK_INT >= 26) {
            ctx.getSystemService<NotificationManager>()?.run {
                createNotificationChannel(createServicesChannel(ctx))
                createNotificationChannel(createEventsChannel(ctx))
            }
        }
    }

    @RequiresApi(26)
    private fun createServicesChannel(ctx: Context) = NotificationChannel(
        SERVICES_CHANNEL_ID,
        ctx.getString(R.string.notification_channel_name_services),
        NotificationManager.IMPORTANCE_LOW
    ).apply {
        setShowBadge(false)
        lockscreenVisibility = Notification.VISIBILITY_PUBLIC
    }

    @RequiresApi(26)
    private fun createEventsChannel(ctx: Context) = NotificationChannel(
        EVENTS_CHANNEL_ID,
        ctx.getString(R.string.notification_channel_name_events),
        NotificationManager.IMPORTANCE_LOW
    )

    class Controller internal constructor(ctx: Context, val id: Int) {
        private val manager = NotificationManagerCompat.from(ctx)
        val builder = NotificationCompat.Builder(ctx, SERVICES_CHANNEL_ID).apply {
            setSmallIcon(id).setShowWhen(false)
        }

        @SuppressLint("MissingPermission")
        fun show() = builder.build().also {
            manager.notify(id, it)
        }

        fun hide() = manager.cancel(id)
    }
}
