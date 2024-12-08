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
     * Track playback mode
     */
    enum class TrackMode {
        /// Play track from start to end
        REGULAR,  /// Loop track according to internal information
        LOOPED
    }

    /**
     * Tracks collection playback mode
     */
    enum class SequenceMode {
        /// Play collection from start to end
        ORDERED,  /// Loop collection (if possible)
        LOOPED,  /// Random position
        SHUFFLE
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
     * Track playback mode
     */
    var trackMode: TrackMode

    /**
     * Sequence playback mode
     */
    var sequenceMode: SequenceMode
}
