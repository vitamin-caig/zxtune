package app.zxtune.ui

import android.os.Bundle
import android.text.method.ScrollingMovementMethod
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageButton
import android.widget.TextView
import androidx.core.animation.ValueAnimator
import androidx.fragment.app.Fragment
import app.zxtune.R
import app.zxtune.core.ModuleAttributes
import app.zxtune.device.media.MediaModel
import app.zxtune.ui.utils.UiUtils
import app.zxtune.ui.utils.whenLifecycleStarted

private const val MAX_LINES = 8

class BriefFragment : Fragment() {
    private class DetailsView(private val view: TextView) {
        private var requiredHeight = 0
        private var shown = false

        init {
            view.run {
                height = 0
                movementMethod = ScrollingMovementMethod.getInstance()
                includeFontPadding = false
            }
        }

        fun setText(txt: CharSequence?) {
            requiredHeight = txt?.run {
                view.lineHeight * (1 + count { it == '\n' }).coerceAtMost(MAX_LINES) + view.paddingTop + view.paddingBottom
            } ?: 0
            resize(slow = true)
            if (shown) {
                view.run {
                    animate().alpha(0f).withEndAction {
                        changeText(txt)
                        animate().alpha(1f)
                    }
                }
            } else {
                changeText(txt)
            }
        }

        private fun changeText(txt: CharSequence?) = view.run {
            text = txt
            scrollTo(0, 0)
        }

        fun setVisibility(visible: Boolean) {
            if (shown != visible) {
                shown = visible
                resize()
            }
        }

        private fun resize(slow: Boolean = false) {
            val currentHeight = view.measuredHeight
            val targetHeight = if (shown) requiredHeight else 0
            if (currentHeight != targetHeight) {
                ValueAnimator.ofInt(currentHeight, targetHeight).apply {
                    addUpdateListener {
                        view.height = animatedValue as Int
                    }
                    if (slow) {
                        duration = view.animate().duration * 2
                    }
                }.start()
            }
        }
    }

    private lateinit var title: TextView
    private lateinit var subtitle: TextView
    private lateinit var details: DetailsView
    private lateinit var detailsToggle: ImageButton

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ) = container?.let {
        inflater.inflate(R.layout.brief, it, false).apply {
            title = findViewById(R.id.brief_title)
            subtitle = findViewById(R.id.brief_subtitle)
            details = DetailsView(findViewById(R.id.brief_details))
            detailsToggle = findViewById(R.id.brief_details_toggle)
            detailsToggle.setOnClickListener {
                val state = !isActivated
                isActivated = state
                details.setVisibility(state)
            }
        }
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) =
        MediaModel.of(requireActivity()).run {
            viewLifecycleOwner.whenLifecycleStarted {
                metadata.collect { metadata ->
                    UiUtils.setViewEnabled(view, metadata != null)
                    metadata?.run {
                        title.updateText(description.title)
                        subtitle.updateText(description.subtitle)
                        val strings = getString(ModuleAttributes.STRINGS)
                        details.setText(strings)
                        detailsToggle.updateVisibility(!strings.isNullOrEmpty())
                    }
                }
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

private fun View.updateVisibility(isVisible: Boolean) {
    val nowVisible = visibility == View.VISIBLE && alpha > 0f
    if (nowVisible != isVisible) {
        if (isVisible) {
            visibility = View.VISIBLE
            animate().alpha(1f)
        } else {
            animate().alpha(0f).withEndAction {
                visibility = View.INVISIBLE
            }
        }
    }
}
