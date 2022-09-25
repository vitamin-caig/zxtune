/**
 *
 * @file
 *
 * @brief Playback implementation of SamplesTarget based on AudioTrack functionality
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.device.sound

import android.media.AudioFormat
import android.media.AudioManager
import android.media.AudioTrack
import app.zxtune.Logger
import app.zxtune.sound.SamplesSource
import app.zxtune.sound.SamplesSource.Sample
import app.zxtune.sound.SamplesTarget

class SoundOutputSamplesTarget private constructor(
    private val bufferSize: Int,
    private val target: AudioTrack
) : SamplesTarget {

    override val sampleRate: Int
        get() = target.sampleRate
    override val preferableBufferSize: Int
        get() = bufferSize / Sample.BYTES

    @Synchronized
    override fun start() = target.play()

    @Synchronized
    override fun writeSamples(buffer: ShortArray) {
        var pos = 0
        var toWrite = buffer.size
        while (toWrite != 0) {
            val written = target.write(buffer, pos, toWrite)
            if (written > 0) {
                pos += written
                toWrite -= written
            } else {
                when {
                    target.playState == AudioTrack.PLAYSTATE_STOPPED -> break //drain
                    written < 0 -> throw Exception("Failed to write samples: $written")
                }
            }
        }
    }

    @Synchronized
    override fun stop() = target.stop()

    @Synchronized
    override fun release() {
        target.release()
    }

    companion object {
        private val LOG = Logger(SoundOutputSamplesTarget::class.java.name)
        private const val CHANNEL_OUT = AudioFormat.CHANNEL_OUT_STEREO
        private const val ENCODING = AudioFormat.ENCODING_PCM_16BIT
        private const val DEFAULT_LATENCY = 160 // minimal is ~55
        const val STREAM = AudioManager.STREAM_MUSIC

        @JvmStatic
        fun create(): SamplesTarget {
            //xmp plugin limits max frequency
            val freqRate = AudioTrack.getNativeOutputSampleRate(STREAM).coerceAtMost(48000)
            val minBufSize = AudioTrack.getMinBufferSize(freqRate, CHANNEL_OUT, ENCODING)
            val prefBufSize =
                SamplesSource.Channels.COUNT * Sample.BYTES * (DEFAULT_LATENCY * freqRate / 1000)
            val bufSize = maxOf(minBufSize, prefBufSize)
            LOG.d {
                "Preparing playback. Freq=$freqRate MinBuffer=$minBufSize PrefBuffer=$prefBufSize BufferSize=$bufSize"
            }
            val target =
                AudioTrack(STREAM, freqRate, CHANNEL_OUT, ENCODING, bufSize, AudioTrack.MODE_STREAM)
            return SoundOutputSamplesTarget(bufSize, target)
        }
    }
}
