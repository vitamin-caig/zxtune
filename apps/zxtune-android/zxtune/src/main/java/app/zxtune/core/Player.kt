package app.zxtune.core

import app.zxtune.Releaseable
import app.zxtune.TimeStamp

/**
 * Player interface
 */
interface Player : PropertiesModifier, Releaseable {
    /**
     * @return Position
     */
    /**
     * @param pos Position
     */
    var position: TimeStamp

    /**
     * @param levels Array of levels to store
     * @return Count of actually stored entries
     */
    fun analyze(levels: ByteArray): Int

    /**
     * Render next result.length bytes of sound data
     *
     * @param result Buffer to put data
     * @return Is there more data to render
     */
    fun render(result: ShortArray): Boolean

    /**
     * @return Rendering performance in percents
     */
    val performance: Int

    /**
     * @return Playback progress in percents (may be >100!!!)
     */
    val progress: Int
}
