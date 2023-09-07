package app.zxtune.ui.playlist

import app.zxtune.TimeStamp
import app.zxtune.Util
import app.zxtune.core.Identifier

// public for binding
data class Entry(
    val id: Long,
    val location: Identifier,
    val title: String,
    val author: String,
    val duration: TimeStamp,
) {
    val displayTitle
        get() = Util.formatTrackTitle(title, location)
}
