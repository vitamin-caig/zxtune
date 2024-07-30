package app.zxtune.fs.provider

import android.net.Uri
import android.provider.OpenableColumns
import app.zxtune.core.jni.Api
import app.zxtune.core.jni.DataCallback
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsFile
import org.junit.After
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.argumentCaptor
import org.mockito.kotlin.clearInvocations
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.eq
import org.mockito.kotlin.mock
import org.mockito.kotlin.stub
import org.mockito.kotlin.times
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner
import java.io.IOException
import java.nio.ByteBuffer
import java.nio.channels.WritableByteChannel

@RunWith(RobolectricTestRunner::class)
class FileOperationTest {

    private val fileUri = Uri.parse("schema://host/path?query")
    private val fullUri = fileUri.buildUpon().fragment("subpath").build()
    private val size = 1234L
    private val resolver = mock<Resolver>()
    private val file = mock<VfsFile>()
    private val reader = mock<(VfsFile) -> ByteBuffer>()
    private val api = mock<Api>()
    private val result = mock<WritableByteChannel>()

    @Before
    fun setUp() = clearInvocations(resolver, file, reader, api, result)

    @After
    fun tearDown() = verifyNoMoreInteractions(resolver, file, reader, api, result)

    private fun underTest(uri: Uri = fullUri, projection: Array<String>? = null) =
        FileOperation(uri, size, resolver, projection, reader, api)

    @Test
    fun `not resolved`() {
        underTest().run {
            assertEquals(null, call())
            assertEquals(null, status())
            assertThrows<IOException> { consumeContent(result) }
        }
        verify(resolver, times(2)).resolve(fileUri)
    }

    @Test
    fun `not a file resolved`() {
        val obj = mock<VfsDir>()
        resolver.stub {
            on { resolve(any()) } doReturn obj
        }
        underTest().run {
            assertEquals(null, call())
            assertEquals(null, status())
            assertThrows<IOException> { consumeContent(result) }
        }
        verify(resolver, times(2)).resolve(fileUri)
    }

    @Test
    fun `columns content`() {
        resolver.stub {
            on { resolve(any()) } doReturn file
        }
        underTest().run {
            checkNotNull(call()).run {
                assertEquals(1, count)
                moveToNext()
                assertEquals(2, columnCount)
                assertArrayEquals(
                    arrayOf(OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE), columnNames
                )
                assertEquals("subpath", getString(0))
                assertEquals(size, getLong(1))
            }
            assertEquals(null, status())
        }
        // custom columns
        underTest(
            projection = arrayOf(
                OpenableColumns.SIZE, "Unknown", OpenableColumns.DISPLAY_NAME, "Z"
            )
        ).run {
            checkNotNull(call()).run {
                assertEquals(1, count)
                moveToNext()
                assertEquals(2, columnCount)
                assertArrayEquals(
                    arrayOf(OpenableColumns.SIZE, OpenableColumns.DISPLAY_NAME), columnNames
                )
                assertEquals(size, getLong(0))
                assertEquals("subpath", getString(1))
            }
            assertEquals(null, status())
        }
        verify(resolver, times(2)).resolve(fileUri)
    }

    @Test
    fun `failed to read`() {
        resolver.stub {
            on { resolve(any()) } doReturn file
        }
        reader.stub {
            on { invoke(any()) } doAnswer { throw IOException() }
        }
        underTest().run {
            assertEquals(null, status())
            assertThrows<IOException> { consumeContent(result) }
        }
        verify(resolver).resolve(fileUri)
        verify(reader).invoke(file)
    }

    @Test
    fun `consume file content`() {
        val fileContent = ByteArray(size.toInt() + 10) { idx ->
            (idx + 1).toByte()
        }
        resolver.stub {
            on { resolve(any()) } doReturn file
        }
        reader.stub {
            on { invoke(any()) } doReturn ByteBuffer.wrap(fileContent)
        }
        val out = mock<WritableByteChannel>()
        underTest(uri = fileUri).run {
            assertNull(status())
            consumeContent(out)
        }
        verify(resolver).resolve(fileUri)
        verify(reader).invoke(file)
        // Cannot access to underlying array
        argumentCaptor<ByteBuffer>().run {
            verify(out).write(capture())
            assertEquals(1, allValues.size)
            firstValue.run {
                assertEquals(0, position())
                assertEquals(size, remaining().toLong())
                assert(isReadOnly)
                val result = ByteArray(size.toInt())
                get(result)
                assertArrayEquals(fileContent.sliceArray(0..<size.toInt()), result)
            }
        }
        verifyNoMoreInteractions(out)
    }

    @Test
    fun `consume archive content`() {
        val fileContent = ByteBuffer.allocate(1)
        val packedContent = ByteArray(size.toInt() + 10) { idx ->
            (idx + 1).toByte()
        }
        resolver.stub {
            on { resolve(any()) } doReturn file
        }
        reader.stub {
            on { invoke(any()) } doReturn fileContent
        }
        api.stub {
            on { loadModuleData(any(), any(), any()) } doAnswer {
                it.getArgument<DataCallback>(2).onData(ByteBuffer.wrap(packedContent))
            }
        }
        val out = mock<WritableByteChannel>()
        underTest().run {
            assertNull(status())
            consumeContent(out)
        }
        verify(resolver).resolve(fileUri)
        verify(reader).invoke(file)
        verify(api).loadModuleData(eq(fileContent), eq("subpath"), any())
        // Cannot access to underlying array
        argumentCaptor<ByteBuffer>().run {
            verify(out).write(capture())
            assertEquals(1, allValues.size)
            firstValue.run {
                assertEquals(0, position())
                assertEquals(size, remaining().toLong())
                assert(isReadOnly)
                val result = ByteArray(size.toInt())
                get(result)
                assertArrayEquals(packedContent.sliceArray(0..<size.toInt()), result)
            }
        }
        verifyNoMoreInteractions(out)
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
