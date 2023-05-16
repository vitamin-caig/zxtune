package app.zxtune.ui

import android.os.Bundle
import android.support.v4.media.session.PlaybackStateCompat
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.fragment.app.Fragment
import app.zxtune.R
import app.zxtune.device.media.MediaModel
import app.zxtune.ui.views.SpectrumAnalyzerView

class VisualizerFragment : Fragment() {
    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ) = container?.let {
        inflater.inflate(R.layout.visualizer, it, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) =
        MediaModel.of(requireActivity()).run {
            val analyzer = view.findViewById<SpectrumAnalyzerView>(R.id.spectrum)
            visualizer.observe(viewLifecycleOwner, analyzer::setSource)
            playbackState.observe(viewLifecycleOwner) { state ->
                analyzer.setIsUpdating(PlaybackStateCompat.STATE_PLAYING == state?.state)
            }
        }
}