package app.zxtune.ui

import android.net.Uri
import android.os.Bundle
import android.support.v4.media.MediaMetadataCompat
import android.text.Html
import android.text.TextUtils
import android.text.method.ScrollingMovementMethod
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.TextView
import androidx.fragment.app.Fragment
import app.zxtune.R
import app.zxtune.core.ModuleAttributes
import app.zxtune.device.media.MediaModel
import app.zxtune.ui.utils.UiUtils

class InformationFragment : Fragment() {

    override fun onCreateView(
        inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?
    ) = container?.let {
        inflater.inflate(R.layout.information, it, false)
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) =
        MediaModel.of(requireActivity()).run {
            val properties = Properties(view)
            metadata.observe(viewLifecycleOwner) { metadata ->
                UiUtils.setViewEnabled(view, metadata != null)
                if (metadata != null) {
                    properties.update(metadata)
                }
            }
        }

    private class Properties(view: View) {
        private val content = view.findViewById<TextView>(R.id.information_content)
        private val titleField: String
        private val authorField: String
        private val programField: String
        private val commentField: String
        private val locationField: String

        init {
            with(view.resources) {
                titleField = getString(R.string.information_title)
                authorField = getString(R.string.information_author)
                programField = getString(R.string.information_program)
                commentField = getString(R.string.information_comment)
                locationField = getString(R.string.information_location)
            }
            content.movementMethod = ScrollingMovementMethod.getInstance()
        }

        fun update(metadata: MediaMetadataCompat) {
            val builder = StringBuilder().apply {
                with(metadata.description) {
                    addField(locationField, mediaUri)
                    addField(titleField, title)
                    addField(authorField, subtitle)
                }
                with(metadata) {
                    addField(programField, getString(ModuleAttributes.PROGRAM))
                    addField(commentField, getString(ModuleAttributes.COMMENT))
                    addRawField(getString(ModuleAttributes.STRINGS))
                }
            }
            content.text = builder.asHtml()
            content.scrollTo(0, 0)
        }
    }
}

const val LINEBREAK = "<br>"

private fun StringBuilder.addField(name: String, value: CharSequence?) =
    value.takeUnless { it.isNullOrEmpty() }?.let {
        addAnyField(name, it)
    } ?: this

private fun StringBuilder.addRawField(value: CharSequence?) =
    value.takeUnless { it.isNullOrEmpty() }?.let {
        addAnyRawField(it)
    } ?: this

private fun StringBuilder.addField(name: String, value: Uri?) = value?.let { uri ->
    addField(name, Uri.decode(uri.toString()))
} ?: this

private fun StringBuilder.addAnyField(name: String, value: CharSequence) = apply {
    if (isNotEmpty()) {
        append(LINEBREAK)
    }
    val escaped = TextUtils.htmlEncode(value.toString())
    append("<b><big>${name}:</big></b><br>&nbsp;${escaped}")
}

private fun StringBuilder.addAnyRawField(value: CharSequence) = apply {
    if (isNotEmpty()) {
        append(LINEBREAK)
    }
    val escaped = TextUtils.htmlEncode(value.toString())
    append("<br><tt>${escaped.replace("\n", LINEBREAK)}</tt>")
}

private fun StringBuilder.asHtml() = Html.fromHtml(toString(), Html.FROM_HTML_MODE_LEGACY)
