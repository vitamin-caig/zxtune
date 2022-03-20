/**
 * @file
 * @brief Sound player interface
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.sound

/**
 * Base interface for low-level sound player
 */
interface Player {
    fun setSource(src: SamplesSource)
    fun startPlayback()
    fun stopPlayback()
    val isStarted: Boolean

    fun release()
}
