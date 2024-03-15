package app.zxtune.device.media

import android.app.Application
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.os.SystemClock
import android.support.v4.media.MediaBrowserCompat
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaControllerCompat
import android.support.v4.media.session.PlaybackStateCompat
import androidx.annotation.MainThread
import androidx.fragment.app.FragmentActivity
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.map
import androidx.lifecycle.switchMap
import app.zxtune.Logger
import app.zxtune.R
import app.zxtune.TimeStamp
import app.zxtune.coverart.BitmapLoader
import app.zxtune.coverart.RemoteImage
import app.zxtune.playback.Visualizer
import app.zxtune.rpc.ParcelableBinder
import app.zxtune.rpc.VisualizerProxy

class MediaModel(app: Application) : AndroidViewModel(app) {

    private val mutablePlaybackState = MutableLiveData<PlaybackStateCompat?>()
    private val mutableMetadata = MutableLiveData<MediaMetadataCompat?>()

    val browser: LiveData<MediaBrowserCompat?> = MediaBrowserConnection(app)
    val controller = browser.map {
        it?.run {
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
    }
    val playbackState: LiveData<PlaybackStateCompat?>
        get() = mutablePlaybackState
    val metadata: LiveData<MediaMetadataCompat?>
        get() = mutableMetadata
    val visualizer = controller.map {
        it?.extras?.let { extras ->
            extractBinder(extras, Visualizer::class.java.name)?.let { binder ->
                VisualizerProxy.getClient(binder)
            }
        }
    }
    val playbackPosition = playbackState.switchMap {
        val state = it?.state ?: return@switchMap MutableLiveData(null)
        if (state == PlaybackStateCompat.STATE_PLAYING) {
            PlaybackPositionLiveData(PositionSource(it))
        } else {
            MutableLiveData(fromMediaTime(it.position))
        }
    }
    // TODO: rework to the flow/livedata of ImageSource
    val coverArt by lazy {
        val loader = BitmapLoader("coverart", app, maxSize = 2)
        RemoteImage(loader).apply {
            setStub(R.drawable.background_faded)
        }
    }

    companion object {
        private val LOG = Logger(MediaModel::class.java.name)

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

    private class PlaybackPositionLiveData(
        getPosition: () -> TimeStamp, updatePeriod: TimeStamp = TimeStamp.fromSeconds(1)
    ) : MutableLiveData<TimeStamp>(getPosition()) {
        private val handler = Handler(Looper.getMainLooper())
        private val task = object : Runnable {
            @MainThread
            override fun run() {
                try {
                    value = getPosition()
                    handler.postDelayed(this, updatePeriod.toMilliseconds())
                } catch (e: Exception) {
                    LOG.w(e) { "PlaybackPositionLiveData::UpdateTask" }
                }
            }
        }

        override fun onActive() = task.run()
        override fun onInactive() = handler.removeCallbacks(task)
    }
}

// TODO: extract to device.media.Utils
internal fun fromMediaTime(ts: Long) = TimeStamp.fromMilliseconds(ts)
internal fun TimeStamp.toMediaTime() = toMilliseconds()

internal fun MediaMetadataCompat.getDuration() =
    fromMediaTime(getLong(MediaMetadataCompat.METADATA_KEY_DURATION))
