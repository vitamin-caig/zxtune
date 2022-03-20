package app.zxtune.io

import org.junit.Assert.*
import org.junit.Test
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.io.IOException
import java.nio.ByteBuffer
import java.nio.MappedByteBuffer

class IoTest {
    @Test
    fun testNonexisting() = File("nonexisting_name").let { file ->
        assertFalse(file.exists())
        expectThrowsIOException { Io.readFrom(file) }
        expectThrowsIOException {
            //really FileInputStream ctor throws
            Io.readFrom(FileInputStream(file))
        }
    }

    @Test
    fun testEmpty() = createFile(0, 0).let { file ->
        expectThrowsIOException { Io.readFrom(file) }
        FileInputStream(file).let { stream ->
            expectThrowsIOException { Io.readFrom(stream) }
        }
    }

    @Test
    fun test1k() {
        val size = 1024
        assertTrue(size < Io.INITIAL_BUFFER_SIZE)
        assertTrue(size < Io.MIN_MMAPED_FILE_SIZE)
        val file = createFile(1, size)
        Io.readFrom(file).let { buf ->
            assertTrue(buf.isDirect)
            // Seems like for some versions DirectBuffer is mapped really
            //assertFalse(buf is MappedByteBuffer)
            checkBuffer(buf, 1, size)
        }
        Io.readFrom(FileInputStream(file)).let { buf ->
            assertFalse(buf.isDirect)
            checkBuffer(buf, 1, size)
        }
        Io.readFrom(FileInputStream(file), size.toLong()).let { buf ->
            assertTrue(buf.isDirect)
            checkBuffer(buf, 1, size)
        }
    }

    @Test
    fun test300k() {
        val size = 300 * 1024
        assertTrue(size > Io.INITIAL_BUFFER_SIZE)
        assertTrue(size > Io.MIN_MMAPED_FILE_SIZE)
        val file = createFile(2, size)
        Io.readFrom(file).let { buf ->
            assertTrue(buf.isDirect)
            assertTrue(buf is MappedByteBuffer)
            checkBuffer(buf, 2, size)
        }
        Io.readFrom(FileInputStream(file)).let { buf ->
            assertFalse(buf.isDirect)
            checkBuffer(buf, 2, size)
        }
        Io.readFrom(FileInputStream(file), size.toLong()).let { buf ->
            assertTrue(buf.isDirect)
            checkBuffer(buf, 2, size)
        }
    }

    @Test
    fun testWrongSizeHint() = createFile(3, 2 * 1024).let { file ->
        checkBuffer(Io.readFrom(FileInputStream(file)), 3, 2 * 1024)
        expectThrowsIOException { Io.readFrom(FileInputStream(file), 1024) }
        expectThrowsIOException { Io.readFrom(FileInputStream(file), (3 * 1024).toLong()) }
    }
}

private fun expectThrowsIOException(cmd: () -> Unit) {
    try {
        cmd()
        fail("Unreachable")
    } catch (e: IOException) {
        assertNotNull("Thrown exception", e)
    }
}

private fun generate(fill: Int, size: Int, obj: File) = FileOutputStream(obj).use { out ->
    repeat(size) {
        out.write(fill)
    }
}

private fun createFile(fill: Int, size: Int) = File.createTempFile("test", "io").apply {
    if (size != 0) {
        generate(fill, size, this)
    }
    assertTrue(canRead())
    assertEquals(size.toLong(), length())
}

private fun checkBuffer(buf: ByteBuffer, fill: Int, size: Int) {
    assertEquals(size.toLong(), buf.limit().toLong())
    for (idx in 0 until size) {
        assertEquals(fill.toLong(), buf[idx].toLong())
    }
}
