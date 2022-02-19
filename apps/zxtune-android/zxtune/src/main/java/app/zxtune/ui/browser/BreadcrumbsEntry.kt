package app.zxtune.ui.browser

import android.net.Uri
import androidx.annotation.DrawableRes

// public for binding
data class BreadcrumbsEntry(
    val uri: Uri,
    val title: String,
    @field:DrawableRes val icon: Int?
)
