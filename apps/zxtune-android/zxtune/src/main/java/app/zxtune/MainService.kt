package app.zxtune

import android.app.Service
import android.content.Context
import android.content.Intent
import android.os.Bundle
import android.support.v4.media.MediaBrowserCompat
import android.support.v4.media.session.MediaSessionCompat
import androidx.media.MediaBrowserServiceCompat
import androidx.media.session.MediaButtonReceiver
import app.zxtune.analytics.Analytics
import app.zxtune.core.jni.Api
import app.zxtune.device.media.AudioFocusConnection
import app.zxtune.device.media.MediaSessionControl
import app.zxtune.device.media.NoisyAudioConnection
import app.zxtune.device.ui.StatusNotification
import app.zxtune.device.ui.WidgetHandler
import app.zxtune.playback.service.PlaybackServiceLocal
import app.zxtune.preferences.Preferences
import app.zxtune.preferences.Preferences.getDefaultSharedPreferences
import app.zxtune.preferences.SharedPreferencesBridge.subscribe
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.SupervisorJob
import kotlinx.coroutines.launch

class MainService : MediaBrowserServiceCompat() {

    // TODO: remove when LifecycleService used
    private val job = SupervisorJob()
    private val scope = CoroutineScope(Dispatchers.IO + job)

    // bind to instance
    private val trace = Analytics.StageDurationTrace.create("MainService2")
    private val delegate by lazy {
        Delegate(this, trace, scope).apply {
            sessionToken = session.sessionToken
            weakDelegate = this
        }
    }
    private var weakDelegate: Delegate? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        LOG.d { "onStartCommand(${intent})" }
        when (intent?.action) {
            Intent.ACTION_MEDIA_BUTTON -> MediaButtonReceiver.handleIntent(delegate.session, intent)
        }
        return super.onStartCommand(intent, flags, startId)
    }

    override fun onGetRoot(
        clientPackageName: String, clientUid: Int, rootHints: Bundle?
    ): BrowserRoot? {
        LOG.d { "onGetRoot(${clientPackageName})" }
        return if (clientPackageName == packageName) {
            delegate // preload
            BrowserRoot(getString(R.string.app_name), null)
        } else {
            null
        }
    }

    override fun onLoadChildren(
        parentId: String, result: Result<List<MediaBrowserCompat.MediaItem>>
    ) = result.sendError(null)

    override fun onDestroy() {
        weakDelegate?.release()
        super.onDestroy()
    }

    override fun onTaskRemoved(rootIntent: Intent?) {
        super.onTaskRemoved(rootIntent)
        weakDelegate?.stop()
        stopSelf()
    }

    private class Delegate(
        svc: Service, trace: Analytics.BaseTrace, private val scope: CoroutineScope
    ) {

        private val resources = ArrayList<Releaseable>()
        private val service: PlaybackServiceLocal
        val session: MediaSessionCompat

        init {
            LOG.d { "Initialize" }
            trace.beginMethod("initialize")
            val ctx = svc.applicationContext
            loadJni(ctx)
            trace.checkpoint("jni")
            PlaybackServiceLocal(ctx, Preferences.getDataStore(ctx)).apply {
                service = this
                addResource(this)
            }
            trace.checkpoint("svc")
            session = createSession(ctx)
            addResource(StatusNotification.connect(svc, session))
            addResource(WidgetHandler.connect(ctx, session))
            trace.checkpoint("cbs")
            service.restoreSession()
            trace.checkpoint("session")
            trace.endMethod()
        }

        fun stop() = service.playbackControl.stop()

        fun release() = resources.forEach(Releaseable::release)

        @Synchronized
        private fun addResource(res: Releaseable) {
            resources.add(res)
        }

        private fun loadJni(ctx: Context) = scope.launch {
            val api = Api.load().await()
            runCatching {
                LOG.d { "JNI is ready" }
                val prefs = getDefaultSharedPreferences(ctx);
                val options = api.getOptions()
                addResource(subscribe(prefs, options))
            }.onFailure {
                LOG.w(it) { "Failed to connect native options" }
            }
        }

        private fun createSession(ctx: Context): MediaSessionCompat {
            addResource(AudioFocusConnection.create(ctx, service))
            addResource(NoisyAudioConnection.create(ctx, service))
            val ctrl = MediaSessionControl.subscribe(ctx, service).also {
                addResource(it)
            }
            return ctrl.session
        }
    }

    companion object {
        private val TAG = MainService::class.java.name
        private val LOG = Logger(TAG)

        @JvmStatic
        val CUSTOM_ACTION_ADD_CURRENT = "$TAG.CUSTOM_ACTION_ADD_CURRENT"

        @JvmStatic
        val CUSTOM_ACTION_ADD = "$TAG.CUSTOM_ACTION_ADD"

        @JvmStatic
        fun createIntent(ctx: Context, action: String?) =
            Intent(ctx, MainService::class.java).setAction(action)
    }
}
