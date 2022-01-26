package app.zxtune.fs.provider

import android.net.Uri
import app.zxtune.fs.TestDir
import app.zxtune.fs.TestFile
import app.zxtune.utils.ProgressCallback
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class ResolveOperationTest {

    private val URI1 = mock<Uri>()
    private val resolver = mock<Resolver>()
    private val schema = mock<SchemaSource>()
    private val file = TestFile(1, "unused")
    private val fileObject =
        Schema.Listing.File(file.uri, file.name, file.description, file.size, null, null)
    private val dir = TestDir(2)
    private val dirObject = Schema.Listing.Dir(dir.uri, dir.name, dir.description, null, false)

    @Before
    fun setUp() = reset(resolver, schema)

    @After
    fun tearDown() = verifyNoMoreInteractions(resolver, schema)

    @Test
    fun `not resolved`() {
        with(ResolveOperation(URI1, resolver, schema)) {
            assertEquals(null, call())
            status().run {
                assertEquals(1, count)
                moveToFirst()
                assertEquals(
                    Schema.Status.Progress.createIntermediate(),
                    Schema.Object.parse(this) as Schema.Status.Progress
                )
            }
        }
        verify(resolver).resolve(eq(URI1), any())
    }

    @Test
    fun `resolve file no progress`() {
        resolver.stub {
            on { resolve(any(), any()) } doReturn file
        }
        schema.stub {
            on { resolved(any()) } doReturn fileObject
        }
        with(ResolveOperation(file.uri, resolver, schema)) {
            call()!!.run {
                assertEquals(1, count)
                moveToFirst()
                assertEquals(fileObject, Schema.Object.parse(this) as Schema.Listing.File)
            }
            status().run {
                assertEquals(1, count)
                moveToFirst()
                assertEquals(
                    Schema.Status.Progress.createIntermediate(),
                    Schema.Object.parse(this) as Schema.Status.Progress
                )
            }
        }
        verify(resolver).resolve(eq(file.uri), any())
        verify(schema).resolved(file)
    }

    @Test
    fun `resolve dir with progress`() {
        resolver.stub {
            on { resolve(any(), any()) } doAnswer {
                it.getArgument<ProgressCallback>(1).onProgressUpdate(12, 34)
                dir
            }
        }
        schema.stub {
            on { resolved(any()) } doReturn dirObject
        }
        with(ResolveOperation(dir.uri, resolver, schema)) {
            call()!!.run {
                assertEquals(1, count)
                moveToFirst()
                assertEquals(dirObject, Schema.Object.parse(this) as Schema.Listing.Dir)
            }
            status().run {
                assertEquals(1, count)
                moveToFirst()
                assertEquals(
                    Schema.Status.Progress(12, 34),
                    Schema.Object.parse(this) as Schema.Status.Progress
                )
            }
        }
        verify(resolver).resolve(eq(dir.uri), any())
        verify(schema).resolved(dir)
    }

    @Test
    fun `resolved unknown object`() {
        resolver.stub {
            on { resolve(any(), any()) } doReturn file
        }
        with(ResolveOperation(file.uri, resolver, schema)) {
            call()!!.run {
                assertEquals(0, count)
            }
            status().run {
                assertEquals(1, count)
                moveToFirst()
                assertEquals(
                    Schema.Status.Progress.createIntermediate(),
                    Schema.Object.parse(this) as Schema.Status.Progress
                )
            }
        }
        verify(resolver).resolve(eq(file.uri), any())
        verify(schema).resolved(file)
    }
}
