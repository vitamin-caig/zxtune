package app.zxtune.io

import android.os.Build
import androidx.annotation.RequiresApi
import app.zxtune.Logger
import java.io.*
import java.nio.ByteBuffer
import java.nio.channels.FileChannel
import java.nio.file.Files
import java.nio.file.StandardCopyOption

private val LOG = Logger(Io::class.java.name)

object Io {

    const val MIN_MMAPED_FILE_SIZE = 131072
    const val INITIAL_BUFFER_SIZE = 262144

    @JvmStatic
    @Throws(IOException::class)
    fun readFrom(file: File) = readFrom(FileInputStream(file))

    @JvmStatic
    @Throws(IOException::class)
    fun readFrom(stream: FileInputStream) = stream.use {
        stream.channel.use {
            readFrom(it)
        }
    }

    private fun readFrom(channel: FileChannel): ByteBuffer {
        if (channel.size() >= MIN_MMAPED_FILE_SIZE) {
            var retry = 1
            while (true) {
                try {
                    return readMemoryMapped(channel)
                } catch (e: IOException) {
                    if (retry == 1) {
                        LOG.w(e) { "Failed to read using MMAP. Cleanup memory" }
                        //http://stackoverflow.com/questions/8553158/prevent-outofmemory-when-using-java-nio-mappedbytebuffer
                        System.gc()
                        System.runFinalization()
                    } else {
                        LOG.w(e) { "Failed to read using MMAP. Fallback" }
                        break
                    }
                }
                ++retry
            }
        }
        return readDirectArray(channel)
    }

    private fun readMemoryMapped(channel: FileChannel) =
        channel.map(FileChannel.MapMode.READ_ONLY, 0, validateSize(channel.size()))

    private fun readDirectArray(channel: FileChannel) = allocateDirectBuffer(channel.size()).apply {
        channel.read(this)
        position(0)
    }

    private fun validateSize(size: Long) = if (size != 0L) size else throw IOException("Empty file")

    @JvmStatic
    @Throws(IOException::class)
    fun readFrom(stream: InputStream): ByteBuffer = stream.use {
        var buffer = reallocate(null)
        var size = 0
        while (true) {
            size = readPartialContent(stream, buffer, size)
            buffer = if (size == buffer.size) {
                reallocate(buffer)
            } else {
                break
            }
        }
        validateSize(size.toLong())
        return ByteBuffer.wrap(buffer, 0, size)
    }

    @JvmStatic
    fun readFrom(stream: InputStream, size: Long): ByteBuffer = stream.use {
        val result = allocateDirectBuffer(size)
        val buffer = reallocate(null)
        var totalSize = 0L
        while (true) {
            val partSize = readPartialContent(stream, buffer, 0)
            if (partSize != 0) {
                totalSize += partSize
                if (totalSize > size) {
                    throw IOException("File size mismatch")
                }
                result.put(buffer, 0, partSize)
                if (partSize == buffer.size) {
                    continue
                }
            }
            break
        }
        if (totalSize != size) {
            throw IOException("File size mismatch")
        }
        result.position(0)
        return result
    }

    private fun readPartialContent(stream: InputStream, buffer: ByteArray, offset: Int): Int {
        var pos = offset
        val len = buffer.size
        while (pos < len) {
            val chunk = stream.read(buffer, pos, len - pos)
            if (chunk < 0) {
                break
            }
            pos += chunk
        }
        return pos
    }

    private fun reallocate(buf: ByteArray?): ByteArray {
        var retry = 1
        while (true) {
            try {
                return buf?.copyOf(buf.size * 3 / 2) ?: ByteArray(INITIAL_BUFFER_SIZE)
            } catch (err: OutOfMemoryError) {
                if (retry == 1) {
                    LOG.d { "Retry reallocate call for OOM" }
                    System.gc()
                } else {
                    throw IOException(err)
                }
            }
            ++retry
        }
    }

    private fun allocateDirectBuffer(size: Long): ByteBuffer {
        validateSize(size)
        var retry = 1
        while (true) {
            try {
                return ByteBuffer.allocateDirect(size.toInt())
            } catch (err: OutOfMemoryError) {
                if (retry == 1) {
                    LOG.d { "Retry reallocate call for OOM" }
                    System.gc()
                } else {
                    throw IOException(err)
                }
            }
            ++retry
        }
    }

    @JvmStatic
    fun copy(source: InputStream, out: OutputStream): Long = source.use { src ->
        val buffer = reallocate(null)
        var total = 0L
        while (true) {
            val size = readPartialContent(src, buffer, 0)
            out.write(buffer, 0, size)
            total += size.toLong()
            if (size != buffer.size) {
                break
            }
        }
        return total
    }

    @JvmStatic
    fun touch(file: File) {
        if (!file.setLastModified(System.currentTimeMillis())) {
            touchFallback(file)
        }
    }

    private fun touchFallback(file: File) = try {
        RandomAccessFile(file, "rw").use {
            val size = it.length()
            it.setLength(size + 1)
            it.setLength(size)
        }
    } catch (e: IOException) {
        LOG.w(e) { "Failed to update file timestamp" }
    }

    @JvmStatic
    fun rename(oldName: File, newName: File) = if (Build.VERSION.SDK_INT >= 26) {
        renameViaFiles(oldName, newName)
    } else {
        (oldName.renameTo(newName)
                || newName.exists() && newName.delete() && oldName.renameTo(newName))
    }

    @RequiresApi(26)
    private fun renameViaFiles(oldName: File, newName: File) = try {
        Files.move(
            oldName.toPath(),
            newName.toPath(),
            StandardCopyOption.ATOMIC_MOVE,
            StandardCopyOption.REPLACE_EXISTING
        )
        true
    } catch (e: Exception) {
        LOG.w(e) { "Failed to rename '${oldName.absolutePath}' -> '${newName.absolutePath}'" }
        false
    }

    @JvmStatic
    fun createByteBufferInputStream(buf: ByteBuffer): InputStream = object : InputStream() {
        override fun available() = buf.remaining()

        override fun mark(readLimit: Int) {
            buf.mark()
        }

        override fun reset() {
            buf.reset()
        }

        override fun markSupported() = true

        override fun read() = if (buf.hasRemaining()) buf.get().toInt() else -1

        override fun read(bytes: ByteArray, off: Int, len: Int) = if (buf.hasRemaining()) {
            val toRead = minOf(len, buf.remaining())
            buf[bytes, off, toRead]
            toRead
        } else {
            -1
        }
    }
}
