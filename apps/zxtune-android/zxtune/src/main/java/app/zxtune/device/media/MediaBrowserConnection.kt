package app.zxtune.device.media

import android.content.ComponentName
import android.content.Context
import android.os.Bundle
import android.support.v4.media.MediaBrowserCompat
import androidx.annotation.VisibleForTesting
import androidx.lifecycle.LiveData
import app.zxtune.MainService
import java.util.concurrent.atomic.AtomicBoolean

typealias MakeBrowser = (Context, ComponentName, MediaBrowserCompat.ConnectionCallback, Bundle?) -> MediaBrowserCompat

class MediaBrowserConnection @VisibleForTesting constructor(
    ctx: Context, makeBrowser: MakeBrowser
) : LiveData<MediaBrowserCompat?>() {

    constructor(ctx: Context) : this(ctx, ::MediaBrowserCompat)

    private val browser = makeBrowser(
        ctx,
        ComponentName(ctx, MainService::class.java),
        object : MediaBrowserCompat.ConnectionCallback() {
            override fun onConnected() = connected()
            override fun onConnectionSuspended() = disconnected()
            override fun onConnectionFailed() = disconnected()
        },
        null
    )
    private var connectionInProgress = AtomicBoolean(false)

    override fun onActive() {
        if (connectionInProgress.compareAndSet(false, true)) {
            browser.connect()
        }
    }

    override fun onInactive() {
        browser.disconnect()
        disconnected()
    }

    private fun connected(): Unit = publish(browser)
    private fun disconnected() = publish(null)
    private fun publish(obj: MediaBrowserCompat?) {
        connectionInProgress.set(false)
        value = obj
    }
}
