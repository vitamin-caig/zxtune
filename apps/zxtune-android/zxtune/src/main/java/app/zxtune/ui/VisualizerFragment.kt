package app.zxtune.ui

import android.content.Context
import android.net.Uri
import android.os.Bundle
import android.support.v4.media.MediaMetadataCompat
import android.support.v4.media.session.PlaybackStateCompat
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import androidx.core.view.isVisible
import androidx.fragment.app.Fragment
import androidx.lifecycle.DefaultLifecycleObserver
import androidx.lifecycle.LifecycleOwner
import app.zxtune.R
import app.zxtune.analytics.Analytics
import app.zxtune.device.media.MediaModel
import app.zxtune.preferences.Preferences
import app.zxtune.ui.utils.whenLifecycleStarted
import app.zxtune.ui.views.SpectrumAnalyzerView
import app.zxtune.utils.ifNotNulls
import kotlinx.coroutines.launch

class VisualizerFragment : Fragment() {
    private val state by lazy {
        VisibilityState(requireContext())
    }
    private lateinit var analyzer: SpectrumAnalyzerView

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ) = container?.let {
        inflater.inflate(R.layout.visualizer, it, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) =
        MediaModel.of(requireActivity()).run {
            analyzer = view.findViewById(R.id.spectrum)
            viewLifecycleOwner.whenLifecycleStarted {
                launch {
                    visualizer.collect(analyzer::setSource)
                }
                launch {
                    playbackState.collect { state ->
                        analyzer.setIsPlaying(PlaybackStateCompat.STATE_PLAYING == state?.state)
                    }
                }
                // TODO: move to model
                launch {
                    metadata.collect { meta ->
                        if (meta == null) {
                            return@collect
                        }
                        var type: String? = null
                        val uri = meta.getString(MediaMetadataCompat.METADATA_KEY_ART_URI)?.also {
                            type = "coverart"
                        } ?: meta.getString(
                            MediaMetadataCompat.METADATA_KEY_ALBUM_ART_URI
                        )?.also {
                            type = "albumart"
                        }
                        if (coverArt.setUri(uri?.let { Uri.parse(it) })) {
                            ifNotNulls(type, meta.description?.mediaUri) { _, source ->
                                Analytics.sendEvent("ui/image", "type" to type, "uri" to source)
                            }
                        }
                    }
                }
            }
            viewLifecycleOwner.lifecycle.addObserver(object : DefaultLifecycleObserver {
                private val imageView: ImageView = view.findViewById(R.id.coverart)
                override fun onStart(owner: LifecycleOwner) = coverArt.bindTo(imageView)
                override fun onStop(owner: LifecycleOwner) = coverArt.bindTo(null)
                override fun onDestroy(owner: LifecycleOwner) =
                    viewLifecycleOwner.lifecycle.removeObserver(this)
            })
            view.setOnClickListener {
                changeState()
            }
            setIsVisible(true)
        }

    private fun changeState() {
        state.toggleNextState()
        setIsVisible(true)
    }

    fun setIsVisible(isVisible: Boolean) {
        analyzer.isVisible = state.isEnabled
        analyzer.setIsUpdating(isVisible)
    }

    private class VisibilityState(ctx: Context) {
        private val prefs = Preferences.getDataStore(ctx)
        private var value = prefs.getInt(PREF_KEY, SPECTRUM)
            set(value) {
                field = value
                prefs.putInt(PREF_KEY, value)
            }

        val isEnabled
            get() = value != OFF

        fun toggleNextState() {
            value = nextOf(value)
        }

        companion object {
            const val OFF = 0
            const val SPECTRUM = 1
            const val PREF_KEY = "ui.visualizer"

            private fun nextOf(state: Int) = when (state) {
                OFF -> SPECTRUM
                else -> OFF
            }
        }
    }
}
