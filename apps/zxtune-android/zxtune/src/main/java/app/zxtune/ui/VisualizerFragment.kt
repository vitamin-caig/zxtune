package app.zxtune.ui

import android.content.Context
import android.os.Bundle
import android.support.v4.media.session.PlaybackStateCompat
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import androidx.core.view.isVisible
import androidx.fragment.app.Fragment
import androidx.lifecycle.lifecycleScope
import app.zxtune.R
import app.zxtune.coverart.withFadeout
import app.zxtune.device.media.MediaModel
import app.zxtune.playback.stubs.VisualizerStub
import app.zxtune.preferences.Preferences
import app.zxtune.ui.utils.whenLifecycleStarted
import app.zxtune.ui.views.SpectrumAnalyzerView
import app.zxtune.utils.ifNotNulls
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.channels.awaitClose
import kotlinx.coroutines.channels.onSuccess
import kotlinx.coroutines.flow.SharingStarted
import kotlinx.coroutines.flow.callbackFlow
import kotlinx.coroutines.flow.collectIndexed
import kotlinx.coroutines.flow.collectLatest
import kotlinx.coroutines.flow.combine
import kotlinx.coroutines.flow.distinctUntilChanged
import kotlinx.coroutines.flow.stateIn
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

class VisualizerFragment : Fragment() {
    private lateinit var analyzer: SpectrumAnalyzerView

    private val scope
        get() = viewLifecycleOwner.lifecycleScope

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ) = container?.let {
        inflater.inflate(R.layout.visualizer, it, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) =
        MediaModel.of(requireActivity()).run {
            analyzer = view.findViewById(R.id.spectrum)
            // TODO: use clicks stream, state may switch different modes
            val state = callbackFlow {
                val storage = StateStorage(requireContext())
                var state = storage.load()
                send(state)
                view.setOnClickListener {
                    val next = state.nextOf()
                    // failed on busy, avoid hanged clicks
                    trySend(next).onSuccess {
                        state = next
                        launch {
                            storage.save(next)
                        }
                    }
                }
                awaitClose {
                    view.setOnClickListener(null)
                }
            }.stateIn(scope, SharingStarted.WhileSubscribed(1000), DEFAULT_STATE)
            viewLifecycleOwner.whenLifecycleStarted {
                launch {
                    visualizer.combine(playbackState) { visualizer, playbackState ->
                        ifNotNulls(visualizer, playbackState?.state) { src, state ->
                            if (PlaybackStateCompat.STATE_PLAYING == state) {
                                src
                            } else {
                                VisualizerStub
                            }
                        }
                    }.distinctUntilChanged { old, new -> old === new }.collectLatest { src ->
                        src?.let {
                            analyzer.drawFrom(it)
                        }
                    }
                }
                launch {
                    state.collect {
                        analyzer.isVisible = it.isVisible
                    }
                }
                launch {
                    val imageView = view.findViewById<ImageView>(R.id.coverart)
                    coverArt.collectIndexed { idx, src ->
                        if (0 == idx) {
                            src.applyTo(imageView)
                        } else {
                            imageView.withFadeout {
                                src.applyTo(it)
                            }
                        }
                    }
                }
            }
        }

    // Show/hide tab
    fun setIsVisible(isVisible: Boolean) {
        scope.launch {
            analyzer.setIsUpdating(isVisible)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        analyzer.reportStatistics()
    }

    // TODO: replace with async DataStore
    private class StateStorage(ctx: Context) {
        private val prefs = Preferences.getDataStore(ctx)

        suspend fun load() = withContext(Dispatchers.IO) {
            State.entries.getOrElse(
                prefs.getInt(PREF_KEY, -1)
            ) { DEFAULT_STATE }
        }

        suspend fun save(state: State) = withContext(Dispatchers.IO) {
            prefs.putInt(PREF_KEY, state.ordinal)
        }
    }

    enum class State {
        OFF, SPECTRUM;

        val isVisible
            get() = this != OFF

        fun nextOf() = when (this) {
            OFF -> SPECTRUM
            else -> OFF
        }
    }

    companion object {
        private const val PREF_KEY = "ui.visualizer"
        private val DEFAULT_STATE = State.SPECTRUM
    }
}
