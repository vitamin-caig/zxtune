package app.zxtune.ui

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.fragment.app.Fragment
import app.zxtune.R
import app.zxtune.device.media.MediaModel
import app.zxtune.ui.utils.UiUtils

class BriefFragment : Fragment() {

    private lateinit var title: TextView
    private lateinit var subtitle: TextView

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ) = container?.let {
        inflater.inflate(R.layout.brief, it, false).apply {
            title = findViewById(R.id.brief_title)
            subtitle = findViewById(R.id.brief_subtitle)
        }
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) =
        MediaModel.of(requireActivity()).metadata.observe(viewLifecycleOwner) { metadata ->
            UiUtils.setViewEnabled(view, metadata != null)
            metadata?.run {
                title.updateText(description.title)
                subtitle.updateText(description.subtitle)
            }
        }
}

// TODO: extract to animation utils
private fun TextView.updateText(txt: CharSequence?, hiddenVisibility: Int = View.GONE) {
    val visibleText = if (visibility == View.VISIBLE && alpha > 0f) text else ""
    if (visibleText != txt) {
        animate().alpha(0f).withEndAction {
            if (txt.isNullOrEmpty()) {
                visibility = hiddenVisibility
            } else {
                visibility = View.VISIBLE
                text = txt
                animate().alpha(1f)
            }
        }
    }
}
