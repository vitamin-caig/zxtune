package app.zxtune.device.media

import android.app.Application
import android.content.ComponentName
import android.os.Build
import android.os.Bundle
import android.os.SystemClock
import android.support.v4.media.MediaBrowserCompat
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaControllerCompat
import android.support.v4.media.session.PlaybackStateCompat
import androidx.fragment.app.FragmentActivity
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.viewModelScope
import app.zxtune.Logger
import app.zxtune.MainService
import app.zxtune.R
import app.zxtune.TimeStamp
import app.zxtune.coverart.BitmapLoader
import app.zxtune.coverart.RemoteImage
import app.zxtune.playback.Visualizer
import app.zxtune.rpc.ParcelableBinder
import app.zxtune.rpc.VisualizerProxy
import kotlinx.coroutines.ExperimentalCoroutinesApi
import kotlinx.coroutines.channels.Channel
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.channels.trySendBlocking
import kotlinx.coroutines.flow.Flow
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.flow.buffer
import kotlinx.coroutines.flow.callbackFlow
import kotlinx.coroutines.flow.flatMapLatest
import kotlinx.coroutines.flow.flow
import kotlinx.coroutines.flow.flowOf
import kotlinx.coroutines.flow.map
import kotlinx.coroutines.flow.shareIn

class MediaModel(app: Application) : AndroidViewModel(app) {

    private val mutablePlaybackState = MutableStateFlow<PlaybackStateCompat?>(null)
    private val mutableMetadata = MutableStateFlow<MediaMetadataCompat?>(null)

    private val browser = callbackFlow {
        lateinit var browser: MediaBrowserCompat
        browser = MediaBrowserCompat(
            app,
            ComponentName(app, MainService::class.java),
            object : MediaBrowserCompat.ConnectionCallback() {
                override fun onConnected() {
                    LOG.d { "Connected!" }
                    trySendBlocking(browser)
                }

                override fun onConnectionSuspended() {
                    LOG.d { "Disconnected!" }
                    trySendBlocking(null)
                }

                override fun onConnectionFailed() {
                    LOG.d { "Connection failed" }
                    trySendBlocking(null)
                }
            },
            null
        )
        LOG.d { "Connecting to service" }
        browser.connect()
        awaitClose {
            browser.disconnect()
        }
        LOG.d { "Finished connection" }
    }

    val controller : Flow<MediaControllerCompat?> = browser.map {
        it?.run {
            LOG.d { "Create controller" }
            MediaControllerCompat(app, sessionToken).apply {
                registerCallback(object : MediaControllerCompat.Callback() {
                    override fun onPlaybackStateChanged(state: PlaybackStateCompat?) {
                        mutablePlaybackState.value = state
                    }

                    override fun onMetadataChanged(metadata: MediaMetadataCompat?) {
                        mutableMetadata.value = metadata
                    }
                })
            }
        }.also { ctrl ->
            mutablePlaybackState.value = ctrl?.playbackState
            mutableMetadata.value = ctrl?.metadata
        }
    }.shareIn(viewModelScope, modelSharing, replay = 1)

    val playbackState
        get() = mutablePlaybackState.asStateFlow()
    val metadata
        get() = mutableMetadata.asStateFlow()
    val visualizer = controller.map {
        it?.extras?.let { extras ->
            extractBinder(extras, Visualizer::class.java.name)?.let { binder ->
                VisualizerProxy.getClient(binder)
            }
        }
    }

    // Cold flow with client-side delays
    @OptIn(ExperimentalCoroutinesApi::class)
    val playbackPosition = playbackState.flatMapLatest {
        if (it?.state == PlaybackStateCompat.STATE_PLAYING) {
            flow {
                LOG.d { "Track dynamic position" }
                val src = PositionSource(it)
                while (true) {
                    emit(src())
                }
            }
        } else {
            LOG.d { "Report static position" }
            flowOf(it?.run { fromMediaTime(position) })
        }
    }.buffer(Channel.RENDEZVOUS)

    // TODO: rework to the flow/livedata of ImageSource
    val coverArt by lazy {
        val loader = BitmapLoader("coverart", app, maxSize = 2)
        RemoteImage(loader).apply {
            setStub(R.drawable.background_faded)
        }
    }

    companion object {
        private val LOG = Logger(MediaModel::class.java.name)
        private val modelSharing = SharingStarted.WhileSubscribed(5000)

        @JvmStatic
        fun of(owner: FragmentActivity) = ViewModelProvider(owner)[MediaModel::class.java]

        private fun extractBinder(extras: Bundle, key: String) = if (Build.VERSION.SDK_INT >= 18) {
            extras.getBinder(key)
        } else {
            //required for proper deserialization
            extras.classLoader = ParcelableBinder::class.java.classLoader
            ParcelableBinder.deserialize(extras.getParcelable(key))
        }
    }

    private class PositionSource(state: PlaybackStateCompat) : () -> TimeStamp {
        private val posMs = state.position
        private var speed = state.playbackSpeed
        private var ts = state.lastPositionUpdateTime

        override fun invoke(): TimeStamp {
            val now = SystemClock.elapsedRealtime()
            val newPosMs = posMs + ((now - ts) * speed).toLong()
            return fromMediaTime(newPosMs)
        }
    }
}

// TODO: extract to device.media.Utils
internal fun fromMediaTime(ts: Long) = TimeStamp.fromMilliseconds(ts)
internal fun TimeStamp.toMediaTime() = toMilliseconds()

internal fun MediaMetadataCompat.getDuration() =
    fromMediaTime(getLong(MediaMetadataCompat.METADATA_KEY_DURATION))
