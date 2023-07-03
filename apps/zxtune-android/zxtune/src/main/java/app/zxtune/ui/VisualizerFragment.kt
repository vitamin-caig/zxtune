package app.zxtune.ui

import android.content.Context
import android.os.Bundle
import android.support.v4.media.session.PlaybackStateCompat
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.core.view.isVisible
import androidx.fragment.app.Fragment
import app.zxtune.R
import app.zxtune.device.media.MediaModel
import app.zxtune.preferences.DataStore
import app.zxtune.ui.views.SpectrumAnalyzerView

class VisualizerFragment : Fragment() {
    private val state by lazy {
        VisibilityState(requireContext())
    }
    private lateinit var analyzer : SpectrumAnalyzerView

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ) = container?.let {
        inflater.inflate(R.layout.visualizer, it, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) =
        MediaModel.of(requireActivity()).run {
            analyzer = view.findViewById<SpectrumAnalyzerView>(R.id.spectrum)
            visualizer.observe(viewLifecycleOwner, analyzer::setSource)
            playbackState.observe(viewLifecycleOwner) { state ->
                analyzer.setIsUpdating(PlaybackStateCompat.STATE_PLAYING == state?.state)
            }
            view.setOnClickListener {
                changeState()
            }
            setIsVisible(true)
        }

    private fun changeState() {
        state.toggleNextState()
        setIsVisible(true)
    }

    fun setIsVisible(isVisible : Boolean) {
        analyzer.isVisible = isVisible && state.isEnabled
    }

    private class VisibilityState(ctx: Context) {
        private val prefs = DataStore(ctx)
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
