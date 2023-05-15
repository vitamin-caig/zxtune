package app.zxtune.device.media

import android.app.Application
import android.os.Build
import android.os.Bundle
import android.support.v4.media.MediaBrowserCompat
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.MediaControllerCompat
import android.support.v4.media.session.PlaybackStateCompat
import androidx.fragment.app.FragmentActivity
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.ViewModelProvider
import androidx.lifecycle.map
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

    companion object {
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
}
