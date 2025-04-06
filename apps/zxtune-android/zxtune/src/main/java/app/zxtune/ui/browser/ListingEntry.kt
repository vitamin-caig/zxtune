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
    @DrawableRes val icon: Int?,
    @DrawableRes val additionalIcon: Int?,
) {
    private val isFolder
        get() = details == null

    @get:DrawableRes
    val mainIcon
        get() = icon ?: if (isFolder) R.drawable.ic_browser_folder else R.drawable.ic_browser_file

    companion object {
        fun makeFolder(
            uri: Uri, title: String, description: String, @DrawableRes icon: Int? = null
        ) = ListingEntry(uri, title, description, null, icon, null)

        fun makeFile(
            uri: Uri,
            title: String,
            description: String,
            details: String,
            @DrawableRes typeIcon: Int?,
        ) = ListingEntry(uri, title, description, details, null, typeIcon)
    }
}
