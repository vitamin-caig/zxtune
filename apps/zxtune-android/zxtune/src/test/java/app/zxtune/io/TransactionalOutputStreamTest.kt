package app.zxtune.io

import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import java.io.File
import java.io.FileInputStream
import java.io.InputStream
import java.io.OutputStream

@RunWith(RobolectricTestRunner::class)
class TransactionalOutputStreamTest {
    @Test
    fun `test new empty`() = getNonexistingFilename("tos1").let { file ->
        TransactionalOutputStream(file).use { stream ->
            stream.flush()
        }
        test(file, 0, 0)
    }

    @Test
    fun `test new non-empty`() = getNonexistingFilename("tos2").let { file ->
        TransactionalOutputStream(file).use { stream ->
            fill(stream, 1, 100)
            stream.flush()
        }
        test(file, 1, 100)
    }

    @Test
    fun `test not confirmed`() = getNonexistingFilename("tos3").let { file ->
        TransactionalOutputStream(file).use { stream ->
            fill(stream, 2, 100)
        }
        assertFalse(file.exists())
    }

    @Test
    fun `test overwrite`() {
        `test new non-empty`()
        getExistingFilename("tos2").let { file ->
            test(file, 1, 100)
            TransactionalOutputStream(file).use { stream ->
                fill(stream, 3, 300)
                stream.flush()
            }
            test(file, 3, 300)
        }
    }

    @Test
    fun `test overwrite not confirmed`() {
        `test new non-empty`()
        getExistingFilename("tos2").let { file ->
            test(file, 1, 100)
            TransactionalOutputStream(file).use { stream ->
                fill(stream, 3, 300)
            }
            test(file, 1, 100)
        }
    }
}

private fun getFileName(name: String) = File(System.getProperty("java.io.tmpdir", "."), name)

private fun getNonexistingFilename(name: String) = getFileName(name)
    .also {
        it.delete()
        assertFalse(it.exists())
    }

private fun getExistingFilename(name: String) = getFileName(name).also { assertTrue(it.isFile) }

private fun fill(stream: OutputStream, fill: Int, size: Int) = with(stream) {
    repeat(size) {
        write(fill)
    }
}

private fun test(stream: InputStream, fill: Int, size: Int) = with(stream) {
    repeat(size) {
        assertEquals(fill.toLong(), read().toLong())
    }
    assertEquals(-1, read().toLong())
}

private fun test(file: File, fill: Int, size: Int) = with(file) {
    assertTrue(isFile)
    assertEquals(size.toLong(), length())
    FileInputStream(this).use {
        test(it, fill, size)
    }
}
