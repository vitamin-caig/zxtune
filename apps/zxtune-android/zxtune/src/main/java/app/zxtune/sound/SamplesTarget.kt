/**
 * @file
 * @brief Samples target interface
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.sound

import app.zxtune.Releaseable

interface SamplesTarget : Releaseable {
    /**
     * @return target sample rate in Hz
     */
    val sampleRate: Int

    /**
     * @return buffer size in samples
     */
    val preferableBufferSize: Int

    /**
     * Initialize target
     */
    @Throws(Exception::class)
    fun start()

    /**
     * @param buffer sound data in S16/stereo/interleaved format
     */
    @Throws(Exception::class)
    fun writeSamples(buffer: ShortArray)

    /**
     * Deinitialize target
     */
    @Throws(Exception::class)
    fun stop()
}
