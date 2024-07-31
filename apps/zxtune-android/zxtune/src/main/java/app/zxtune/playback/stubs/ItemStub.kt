/**
 *
 * @file
 *
 * @brief Stub singleton implementation of Item
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playback.stubs

import android.net.Uri
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import app.zxtune.playback.Item

open class ItemStub : Item {
    override val id: Uri
        get() = Uri.EMPTY

    override val dataId: Identifier
        get() = Identifier.EMPTY

    override val title: String
        get() = ""

    override val author: String
        get() = ""

    override val program: String
        get() = ""

    override val comment: String
        get() = ""

    override val strings: String
        get() = ""

    override val duration: TimeStamp
        get() = TimeStamp.EMPTY

    override val size: Long
        get() = 0
}
