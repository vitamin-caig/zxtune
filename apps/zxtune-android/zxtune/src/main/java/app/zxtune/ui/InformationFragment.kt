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
import androidx.lifecycle.ViewModel
import androidx.lifecycle.ViewModelProvider
import app.zxtune.R
import app.zxtune.core.ModuleAttributes

class InformationFragment : DialogFragment(R.layout.information) {

    companion object {
        fun show(activity: FragmentActivity, metadata: MediaMetadataCompat) =
            InformationFragment().apply {
                Model.of(activity).fill(metadata)
            }.show(activity.supportFragmentManager, "information")
    }

    override fun onViewCreated(view: View, savedInstanceState: Bundle?) =
        Model.of(requireActivity()).showTo(view.findViewById(R.id.information_content))

    override fun onCreateDialog(savedInstanceState: Bundle?) =
        super.onCreateDialog(savedInstanceState).also {
            it.setTitle(R.string.properties)
        }

    class Model : ViewModel() {
        companion object {
            fun of(owner: FragmentActivity) = ViewModelProvider(owner)[Model::class.java]
        }

        private lateinit var content: List<Pair<Int, String>>

        fun fill(metadata: MediaMetadataCompat) = with(metadata) {
            content = mutableListOf<Pair<Int, String>>().apply {
                val location = Uri.decode(description.mediaUri?.toString() ?: "")
                add(R.string.information_location to location)
                add(R.string.information_title to getString(ModuleAttributes.TITLE))
                add(R.string.information_author to getString(ModuleAttributes.AUTHOR))
                add(R.string.information_program to getString(ModuleAttributes.PROGRAM))
                add(R.string.information_comment to getString(ModuleAttributes.COMMENT))
                add(0 to getString(ModuleAttributes.STRINGS))
            }
        }

        fun showTo(view: TextView) = StringBuilder().apply {
            val res = view.resources
            for ((key, value) in content) {
                if (key != 0) {
                    addField(res.getString(key), value)
                } else {
                    addRawField(value)
                }
            }
        }.let { content ->
            view.run {
                text = content.asHtml()
                movementMethod = ScrollingMovementMethod.getInstance()
                scrollTo(0, 0)
            }
        }
    }
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
    append("<b><big>${name}:</big></b><br>&nbsp;${escaped}")
}

private fun StringBuilder.addAnyRawField(value: CharSequence) = apply {
    if (isNotEmpty()) {
        append(LINEBREAK)
    }
    val escaped = TextUtils.htmlEncode(value.toString())
    append("<br><tt>${escaped.replace("\n", LINEBREAK)}</tt>")
}

private fun StringBuilder.asHtml() = if (Build.VERSION.SDK_INT >= 24) {
    Html.fromHtml(toString(), Html.FROM_HTML_MODE_LEGACY)
} else {
    @Suppress("DEPRECATION") Html.fromHtml(toString())
}
