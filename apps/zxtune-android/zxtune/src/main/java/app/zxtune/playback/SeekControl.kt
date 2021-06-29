/**
 *
 * @file
 *
 * @brief Seek controller interface
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playback

import app.zxtune.TimeStamp

interface SeekControl {
    val duration: TimeStamp
    var position: TimeStamp
}
