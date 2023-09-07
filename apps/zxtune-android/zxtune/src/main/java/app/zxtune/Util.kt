/**
 *
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune

import app.zxtune.core.Identifier
import java.util.Locale

object Util {
    //! @return "${left}" or "${right}" or "${left} - ${right}" or "${fallback}"
    @JvmStatic
    fun formatTrackTitle(left: String, right: String, fallback: String) = when {
        left.isEmpty() -> right.ifEmpty { fallback }
        right.isEmpty() -> left
        else -> "$left - $right"
    }

    @JvmStatic
    fun formatTrackTitle(title: String, id: Identifier) = if (title.isEmpty()) {
        id.displayFilename
    } else {
        id.trackIndex?.let {
            "$title (#${it})"
        } ?: title
    }

    @JvmStatic
    fun formatSize(v: Long) = if (v < 1024) {
        v.toString()
    } else {
        val z = (63 - v.countLeadingZeroBits()) / 10
        "%.1f%s".format(Locale.US, v.toFloat() / (1 shl z * 10), " KMGTPE"[z])
    }
}
