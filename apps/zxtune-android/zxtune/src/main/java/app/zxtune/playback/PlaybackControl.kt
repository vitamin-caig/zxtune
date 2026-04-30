/**
 * @file
 * @brief Playback controller interface
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playback

interface PlaybackControl {
    /**
     * Current state
     */
    enum class State {
        STOPPED, PLAYING, SEEKING
    }

    /**
     * Activate currently playing item
     */
    fun play()

    /**
     * Stop currently playing item
     */
    fun stop()

    /**
     * Play next item in sequence
     */
    fun next()

    /**
     * Play previous item in sequence
     */
    fun prev()

    /**
     * Track looped playback according to settings
     */
    var trackLooped: Boolean

    /**
     * Sequence shuffled playback
     */
    var shuffledOrder: Boolean
}
