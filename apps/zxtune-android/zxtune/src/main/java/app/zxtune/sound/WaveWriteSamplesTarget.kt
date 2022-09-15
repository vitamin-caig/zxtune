/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.sound

import java.io.RandomAccessFile
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.nio.channels.FileChannel

class WaveWriteSamplesTarget : SamplesTarget {

    companion object {
        private const val SAMPLERATE = 44100
        private const val CHANNELS = 1
        private const val BYTES_PER_SAMPLE = Short.SIZE_BYTES

        private const val EXTENSION = ".wav"

        private fun maybeAddExtension(filename: String) =
            if (filename.endsWith(EXTENSION, ignoreCase = true)) {
                filename
            } else {
                filename + EXTENSION
            }
    }

    private class Header {
        private val content = ByteBuffer.allocate(44).order(ByteOrder.LITTLE_ENDIAN)

        companion object {
            private const val OFFSET_WAVESIZE = 4
            private const val OFFSET_DATA_SIZE = 40
        }

        init {
            with(content) {
                // +0 - signature 'R', 'I', 'F', 'F'
                put(byteArrayOf(0x52, 0x49, 0x46, 0x46))
                // +4 - file size - 8
                putInt(0)
                //+8 - 'W', 'A', 'V', 'E', 'f', 'm', 't', ' '
                put(byteArrayOf(0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20))
                //+16 - chunkSize=16
                putInt(BYTES_PER_SAMPLE * 8)
                //+20 - compression
                putShort(1)
                //+22 - channels
                putShort(CHANNELS.toShort())
                //+24 - samplerate
                putInt(SAMPLERATE)
                //+28 - bytes per sec
                putInt(SAMPLERATE * BYTES_PER_SAMPLE)
                //+32 - align
                putShort(BYTES_PER_SAMPLE.toShort())
                //+34 - bits per sample
                putShort((8 * BYTES_PER_SAMPLE).toShort())
                //+36 - 'd', 'a', 't', 'a',
                put(byteArrayOf(0x64, 0x61, 0x74, 0x61))
                //+40 - data size
                putInt(0)
            }
        }

        val size
            get() = content.capacity()

        fun setDataSize(dataSize: Int): Unit = with(content) {
            position(OFFSET_DATA_SIZE)
            putInt(dataSize)
            position(OFFSET_WAVESIZE)
            putInt(size - 8 + dataSize)
        }

        fun writeTo(file: FileChannel): Unit = with(file) {
            val pos = position()
            position(0)
            write(content.apply { rewind() })
            position(pos)
        }
    }

    private val file: FileChannel
    private val header = Header()
    private lateinit var buffer: ByteBuffer
    private var doneBytes: Int = 0

    constructor(filename: String) : this(
        RandomAccessFile(maybeAddExtension(filename), "rw")
            .channel
    )

    constructor(channel: FileChannel) {
        file = channel
    }

    override val sampleRate = SAMPLERATE
    override val preferableBufferSize = SAMPLERATE //1s buffer

    override fun start() {
        file.position(header.size.toLong())
        doneBytes = 0
    }

    override fun writeSamples(buffer: ShortArray) {
        val inSamples = buffer.size / SamplesSource.Channels.COUNT
        val outBytes = inSamples * SamplesSource.Sample.BYTES
        allocateBuffer(outBytes)
        convertBuffer(buffer)
        file.write(this.buffer.apply { flip() })
        doneBytes += outBytes
    }

    override fun stop() = with(header) {
        setDataSize(doneBytes)
        writeTo(file)
    }

    override fun release() = file.close()

    private fun allocateBuffer(size: Int) {
        if (!this::buffer.isInitialized || buffer.capacity() < size) {
            buffer = ByteBuffer.allocate(size).order(ByteOrder.LITTLE_ENDIAN)
        }
    }

    private fun convertBuffer(input: ShortArray) = with(buffer) {
        clear()
        for (inp in input.indices step SamplesSource.Channels.COUNT) {
            var sample = 0
            for (ch in 0 until SamplesSource.Channels.COUNT) {
                sample += input[inp + ch]
            }
            buffer.putShort((sample / SamplesSource.Channels.COUNT).toShort())
        }
    }
}
