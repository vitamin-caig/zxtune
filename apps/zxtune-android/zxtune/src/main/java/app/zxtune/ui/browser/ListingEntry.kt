package app.zxtune.ui.browser

import android.net.Uri
import androidx.annotation.DrawableRes
import app.zxtune.R

// public for binding
data class ListingEntry(
    val uri: Uri,
    val title: String,
    val description: String,
    val details: String?,
    val icon: Icon?,
    @DrawableRes val additionalIcon: Int?,
) {
    private val isFolder
        get() = details == null

    sealed interface Icon

    @JvmInline
    value class DrawableIcon(@DrawableRes val id: Int) : Icon

    @JvmInline
    value class LoadableIcon(val uri: Uri) : Icon

    @get:DrawableRes
    val mainIcon
        get() = when {
            icon is DrawableIcon -> icon.id
            isFolder -> R.drawable.ic_browser_folder
            else -> R.drawable.ic_browser_file
        }

    companion object {
        fun makeFolder(
            uri: Uri, title: String, description: String, icon: Icon? = null
        ) = ListingEntry(uri, title, description, null, icon, null)

        fun makeFile(
            uri: Uri,
            title: String,
            description: String,
            details: String,
            @DrawableRes typeIcon: Int?,
            icon: Icon? = null
        ) = ListingEntry(uri, title, description, details, icon, typeIcon)
    }
}
