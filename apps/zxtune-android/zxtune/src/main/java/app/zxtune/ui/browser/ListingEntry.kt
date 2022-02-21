package app.zxtune.ui.browser

import android.net.Uri
import androidx.annotation.DrawableRes
import androidx.annotation.IntDef
import app.zxtune.R

// public for binding
data class ListingEntry(
    @Type val type: Int,
    val uri: Uri,
    val title: String,
    val description: String,
    @DrawableRes var icon: Int? = null,
    var details: String? = null,
    var tracks: Int? = null,
    var cached: Boolean? = null,
) {
    @kotlin.annotation.Retention(AnnotationRetention.SOURCE)
    @IntDef(FOLDER, FILE)
    private annotation class Type

    companion object {
        const val FOLDER = 0
        const val FILE = 1

        fun makeFolder(
            uri: Uri,
            title: String,
            description: String,
            @DrawableRes icon: Int? = null
        ) = ListingEntry(
            type = FOLDER,
            uri = uri,
            title = title,
            description = description,
            icon = icon
        )

        fun makeFile(
            uri: Uri,
            title: String,
            description: String,
            details: String,
            tracks: Int? = null,
            cached: Boolean? = null
        ) = ListingEntry(
            type = FILE,
            uri = uri,
            title = title,
            description = description,
            details = details,
            tracks = tracks,
            cached = cached
        )
    }

    @get:DrawableRes
    val displayIcon
        get() = icon ?: when {
            type == FOLDER -> R.drawable.ic_browser_folder
            cached == false -> R.drawable.ic_browser_file_remote
            tracks == null -> R.drawable.ic_browser_file_unknown
            tracks == 0 -> R.drawable.ic_browser_file_unsupported
            tracks == 1 -> R.drawable.ic_browser_file_track
            else -> R.drawable.ic_browser_file_archive
        }
}
