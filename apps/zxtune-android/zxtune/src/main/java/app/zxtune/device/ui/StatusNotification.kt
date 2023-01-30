/**
 * @file
 * @brief Status notification support
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.device.ui

import android.app.Service
import android.content.pm.ServiceInfo
import android.os.Build
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaControllerCompat
import android.support.v4.media.session.MediaSessionCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.support.v4.media.session.PlaybackStateCompat.MediaKeyAction
import androidx.annotation.DrawableRes
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import androidx.media.app.NotificationCompat.MediaStyle
import androidx.media.session.MediaButtonReceiver
import app.zxtune.Logger
import app.zxtune.MainService
import app.zxtune.R

private val LOG = Logger(StatusNotification::class.java.name)

class StatusNotification private constructor(
    private val service: Service, session: MediaSessionCompat
) : MediaControllerCompat.Callback() {

    companion object {
        private const val ACTION_PREV = 0
        private const val ACTION_PLAY_PAUSE = 1
        private const val ACTION_NEXT = 2

        @JvmStatic
        fun connect(service: Service, session: MediaSessionCompat) =
            session.controller.registerCallback(StatusNotification(service, session))

        private fun needFixForHuawei() = Build.MANUFACTURER.contains(
            "huawei",
            ignoreCase = true
        ) && Build.VERSION.SDK_INT < Build.VERSION_CODES.M
    }

    private val notification =
        Notifications.createForService(service, R.drawable.ic_stat_notify_play)
    private var isPlaying = false
    private var isForeground = false

    init {
        notification.builder.run {
            setCategory(NotificationCompat.CATEGORY_TRANSPORT)
            setVisibility(NotificationCompat.VISIBILITY_PUBLIC)
            setContentIntent(session.controller.sessionActivity)
            foregroundServiceBehavior = NotificationCompat.FOREGROUND_SERVICE_IMMEDIATE
            if (!needFixForHuawei()) {
                setStyle(with(MediaStyle()) {
                    setShowActionsInCompactView(ACTION_PREV, ACTION_PLAY_PAUSE, ACTION_NEXT)
                    setMediaSession(session.sessionToken)
                })
            }
        }
        setupActions()
        // Hack for Android O requirement to call Service.startForeground after Context.startForegroundService.
        // Service.startForeground may be called from android.support.v4.media.session.MediaButtonReceiver, but
        // no playback may happens.
        startForeground()
        stopForeground(removeNotification = true)
    }

    private fun setupActions() = notification.builder.run {
        clearActions()
        addAction(createAction(R.drawable.ic_prev, PlaybackStateCompat.ACTION_SKIP_TO_PREVIOUS))
        addAction(if (isPlaying) createPauseAction() else createPlayAction())
        addAction(createAction(R.drawable.ic_next, PlaybackStateCompat.ACTION_SKIP_TO_NEXT))
    }

    private fun createAction(@DrawableRes icon: Int, @MediaKeyAction action: Long) =
        NotificationCompat.Action(
            icon,
            "",
            MediaButtonReceiver.buildMediaButtonPendingIntent(service, action)
        )

    private fun createPauseAction() =
        createAction(R.drawable.ic_pause, PlaybackStateCompat.ACTION_STOP)

    private fun createPlayAction() =
        createAction(R.drawable.ic_play, PlaybackStateCompat.ACTION_PLAY)

    private fun startForeground() = try {
        ContextCompat.startForegroundService(service, MainService.createIntent(service, null))
        if (Build.VERSION.SDK_INT >= 29) {
            service.startForeground(
                notification.id,
                notification.show(),
                ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PLAYBACK
            )
        } else {
            service.startForeground(notification.id, notification.show())
        }
        isForeground = true
    } catch (e: Exception) {
        // TODO: do not exit foreground mode when playback stopped due to focus loss
        LOG.w(e) { "Failed to start foreground service" }
    }

    private fun stopForeground(removeNotification: Boolean = false) {
        notification.show()
        service.stopForeground(removeNotification)
        isForeground = false
    }

    override fun onPlaybackStateChanged(state: PlaybackStateCompat?) = state?.let {
        isPlaying = it.state == PlaybackStateCompat.STATE_PLAYING
        setupActions()
        if (isPlaying) {
            startForeground()
        } else if (isForeground) {
            stopForeground()
        }
    } ?: Unit

    override fun onMetadataChanged(metadata: MediaMetadataCompat) = metadata.description.run {
        notification.builder
            .setContentTitle(title)
            .setContentText(subtitle)
            .setLargeIcon(iconBitmap)
        if (isPlaying) {
            notification.show()
        }
    }
}
