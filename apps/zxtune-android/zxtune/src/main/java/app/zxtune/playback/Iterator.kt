/**
 *
 * @file
 *
 * @brief Playback iterator interface
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playback

interface Iterator {
    /**
     * Retrieve current position' item
     * @return Newly loaded item
     */
    val item: PlayableItem

    /**
     * Move to next position
     * @return true if successfully moved, else getItem will return the same item
     */
    fun next(): Boolean

    /**
     * Move to previous position
     * @return true if successfully moved, else getItem will return the same item
     */
    fun prev(): Boolean
}
