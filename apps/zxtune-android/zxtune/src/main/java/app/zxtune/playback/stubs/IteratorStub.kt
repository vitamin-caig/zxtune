/**
 *
 * @file
 *
 * @brief Stub singleton implementation of Iterator
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playback.stubs

import app.zxtune.playback.Iterator

object IteratorStub : Iterator {
    override val item
        get() = throw IllegalStateException("Should not be called")

    override fun next() = false

    override fun prev() = false

    //TODO: remove
    @JvmStatic
    fun instance() = IteratorStub
}
