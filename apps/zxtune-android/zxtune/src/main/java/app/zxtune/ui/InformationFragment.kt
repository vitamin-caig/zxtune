package app.zxtune.ui

import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.support.v4.media.MediaMetadataCompat
import android.text.Html
import android.text.TextUtils
import android.text.method.ScrollingMovementMethod
import android.view.View
import android.widget.TextView
import androidx.fragment.app.DialogFragment
import androidx.fragment.app.FragmentActivity
import app.zxtune.R
import app.zxtune.core.ModuleAttributes
import app.zxtune.ui.utils.FragmentParcelableProperty

class InformationFragment : DialogFragment(R.layout.information) {

    companion object {
        fun show(activity: FragmentActivity, metadata: MediaMetadataCompat) =
            InformationFragment().apply {
                this.metadata = metadata
            }.show(activity.supportFragmentManager, "information")
    }

    private var metadata by FragmentParcelableProperty<MediaMetadataCompat>()

    private val content by lazy {
        buildContent()
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) =
        view.findViewById<TextView>(R.id.information_content).run {
            text = content
            movementMethod = ScrollingMovementMethod.getInstance()
            scrollTo(0, 0)
        }

    override fun onCreateDialog(savedInstanceState: Bundle?) =
        super.onCreateDialog(savedInstanceState).also {
            it.setTitle(R.string.properties)
        }

    private fun buildContent() = StringBuilder().apply {
        with(metadata) {
            addField(
                getString(R.string.information_location),
                Uri.decode(description.mediaUri?.toString() ?: "")
            )
            addField(getString(R.string.information_title), getString(ModuleAttributes.TITLE))
            addField(getString(R.string.information_author), getString(ModuleAttributes.AUTHOR))
            addField(getString(R.string.information_program), getString(ModuleAttributes.PROGRAM))
            addField(getString(R.string.information_comment), getString(ModuleAttributes.COMMENT))
            addRawField(getString(ModuleAttributes.STRINGS))
        }
    }.asHtml()
}

private const val LINEBREAK = "<br>"

private fun StringBuilder.addField(name: String, value: CharSequence?) =
    value.takeUnless { it.isNullOrEmpty() }?.let {
        addAnyField(name, it)
    } ?: this

private fun StringBuilder.addRawField(value: CharSequence?) =
    value.takeUnless { it.isNullOrEmpty() }?.let {
        addAnyRawField(it)
    } ?: this

private fun StringBuilder.addAnyField(name: String, value: CharSequence) = apply {
    if (isNotEmpty()) {
        append(LINEBREAK)
    }
    val escaped = TextUtils.htmlEncode(value.toString())
    append("<b><big>${name}:</big></b><br>&nbsp;${escaped.replaceLinebreaks()}")
}

private fun StringBuilder.addAnyRawField(value: CharSequence) = apply {
    if (isNotEmpty()) {
        append(LINEBREAK)
    }
    val escaped = TextUtils.htmlEncode(value.toString())
    append("<br><tt>${escaped.replaceLinebreaks()}</tt>")
}

private fun StringBuilder.asHtml() = if (Build.VERSION.SDK_INT >= 24) {
    Html.fromHtml(toString(), Html.FROM_HTML_MODE_LEGACY)
} else {
    @Suppress("DEPRECATION") Html.fromHtml(toString())
}

private fun String.replaceLinebreaks() = replace("\n", LINEBREAK)
