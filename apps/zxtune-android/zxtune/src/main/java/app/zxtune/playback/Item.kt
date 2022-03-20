/**
 * @file
 * @brief Interface for playback item (metadata only)
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playback

import android.net.Uri
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier

interface Item {
    /**
     * @return Unique item identifier - may be equal to dataId in case of plain file playback
     */
    val id: Uri

    /**
     * @return Item's data identifier
     */
    val dataId: Identifier

    /**
     * @return Item's title (possibly empty)
     */
    val title: String

    /**
     * @return Item's author (possibly empty)
     */
    val author: String

    /**
     * @return Item's program information (possibly empty)
     */
    val program: String

    /**
     * @return Item's comment (possibly empty)
     */
    val comment: String

    /**
     * @return Item's internal strings (possibly empty)
     */
    val strings: String

    /**
     * @return Item's duration
     */
    val duration: TimeStamp
}
