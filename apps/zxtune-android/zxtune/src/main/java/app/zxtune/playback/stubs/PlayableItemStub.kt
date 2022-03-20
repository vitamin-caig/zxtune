/**
 *
 * @file
 *
 * @brief Stub singleton implementation of PlayableItem
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playback.stubs

import app.zxtune.playback.PlayableItem

object PlayableItemStub : ItemStub(), PlayableItem {
    override val module
        get() = throw IllegalStateException("Should not be called")

    //TODO: remove
    @JvmStatic
    fun instance() = PlayableItemStub
}
