/**
 *
 * @file
 *
 * @brief Samples source interface
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.sound

import app.zxtune.TimeStamp

interface SamplesSource {
    object Channels {
        const val COUNT = 2
    }

    object Sample {
        const val BYTES = 2
    }

    /**
     * Acquire next sound chunk
     * @param buf result buffer of 16-bit signed interleaved stereo signal
     * @return true if buffer filled, else reset position to initial
     */
    fun getSamples(buf: ShortArray): Boolean

    /**
     * Current playback position
     */
    var position: TimeStamp
}
