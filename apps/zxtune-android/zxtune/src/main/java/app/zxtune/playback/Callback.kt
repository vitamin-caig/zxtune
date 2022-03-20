/**
 *
 * @file
 *
 * @brief Notification callback interface
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playback

import app.zxtune.TimeStamp

interface Callback {
    /**
     * Called after subscription
     */
    fun onInitialState(state: PlaybackControl.State)

    /**
     * Called on new state (may be the same)
     */
    fun onStateChanged(state: PlaybackControl.State, pos: TimeStamp)

    /**
     * Called on active item change
     */
    fun onItemChanged(item: Item)

    /**
     * Called on error happends
     */
    fun onError(e: String)
}
