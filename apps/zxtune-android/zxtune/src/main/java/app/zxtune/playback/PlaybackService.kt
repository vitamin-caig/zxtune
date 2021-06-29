/**
 *
 * @file
 *
 * @brief Playback service interface
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playback

import app.zxtune.Releaseable

interface PlaybackService {
    val playbackControl: PlaybackControl
    val seekControl: SeekControl
    val visualizer: Visualizer

    fun subscribe(cb: Callback): Releaseable
}
