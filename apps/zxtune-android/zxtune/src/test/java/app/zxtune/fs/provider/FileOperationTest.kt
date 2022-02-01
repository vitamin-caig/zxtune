package app.zxtune.fs.provider

import android.net.Uri
import android.provider.OpenableColumns
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsExtensions
import app.zxtune.fs.VfsFile
import org.junit.After
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner
import java.io.File
import java.io.IOException

@RunWith(RobolectricTestRunner::class)
class FileOperationTest {

    private val uri = Uri.parse("schema://host/path?query")
    private val resolver = mock<Resolver>()

    @Before
    fun setUp() = clearInvocations(resolver)

    @After
    fun tearDown() = verifyNoMoreInteractions(resolver)

    @Test
    fun `not resolved`() {
        with(FileOperation(uri, resolver, null)) {
            assertEquals(null, call())
            assertEquals(null, status())
            assertThrows<IllegalArgumentException> { openFile("z") }
            assertThrows<IOException> { openFile("r") }
        }
        verify(resolver, times(2)).resolve(uri)
    }

    @Test
    fun `not a file resolved`() {
        val obj = mock<VfsDir>()
        resolver.stub {
            on { resolve(any()) } doReturn obj
        }
        with(FileOperation(uri, resolver, null)) {
            assertEquals(null, call())
            assertEquals(null, status())
            assertThrows<IllegalArgumentException> { openFile("w") }
            assertThrows<IOException> { openFile("r") }
        }
        verify(resolver, times(2)).resolve(uri)
    }

    @Test
    fun `local dir resolved`() {
        val local = mock<File>()
        val obj = mock<VfsFile> {
            on { getExtension(VfsExtensions.FILE) } doReturn local
        }
        resolver.stub {
            on { resolve(any()) } doReturn obj
        }
        with(FileOperation(uri, resolver, null)) {
            assertEquals(null, call())
            assertEquals(null, status())
            assertThrows<IllegalArgumentException> { openFile("r+") }
            assertThrows<IOException> { openFile("r") }
        }
        inOrder(resolver, obj, local) {
            verify(resolver, times(2)).resolve(uri)
            verify(obj).getExtension(VfsExtensions.FILE)
            verify(local).isFile
        }
    }

    @Test
    fun `local file resolved`() {
        val displayName = "Some string"
        val size = 1234L
        val local = mock<File> {
            on { isFile } doReturn true
            on { length() } doReturn size
        }
        val obj = mock<VfsFile> {
            on { name } doReturn displayName
            on { getExtension(VfsExtensions.FILE) } doReturn local
        }
        resolver.stub {
            on { resolve(any()) } doReturn obj
        }
        with(FileOperation(uri, resolver, null)) {
            call()!!.run {
                assertEquals(1, count)
                moveToNext()
                assertEquals(2, columnCount)
                assertArrayEquals(
                    arrayOf(OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE),
                    columnNames
                )
                assertEquals(displayName, getString(0))
                assertEquals(size, getLong(1))
            }
            assertEquals(null, status())
            assertEquals(local, openFileInternal("r"))
        }
        // custom columns
        with(
            FileOperation(
                uri,
                resolver,
                arrayOf(OpenableColumns.SIZE, "Unknown", OpenableColumns.DISPLAY_NAME, "Z")
            )
        ) {
            call()!!.run {
                assertEquals(1, count)
                moveToNext()
                assertEquals(2, columnCount)
                assertArrayEquals(
                    arrayOf(OpenableColumns.SIZE, OpenableColumns.DISPLAY_NAME),
                    columnNames
                )
                assertEquals(size, getLong(0))
                assertEquals(displayName, getString(1))
            }
            assertEquals(null, status())
        }
        inOrder(resolver, obj, local) {
            verify(resolver, times(3)).resolve(uri)
            verify(obj).getExtension(VfsExtensions.FILE)
            verify(local).isFile
        }
    }
}

// TODO: migrate to JUnit after upgrade
internal inline fun <reified T> assertThrows(cmd: () -> Unit): T = try {
    cmd()
    fail("${T::class.java.name} was not thrown")
} catch (e: Throwable) {
    when (e) {
        is AssertionError -> throw e
        is T -> e
        else -> fail("Unexpected exception ${e::class.java.name}: ${e.message}")
    }
}

fun fail(msg: String): Nothing = throw AssertionError(msg)
