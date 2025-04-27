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
import org.mockito.kotlin.any
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.eq
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.reset
import org.mockito.kotlin.stub
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class ResolveOperationTest {

    private val URI1 = mock<Uri>()
    private val resolver = mock<Resolver>()
    private val schema = mock<SchemaSource>()
    private val callback = mock<AsyncQueryOperation.Callback>()
    private val file = TestFile(1, "unused")
    private val fileObject = Schema.Listing.File(
        file.uri, file.name, file.description, file.size, Schema.Listing.File.Type.UNKNOWN
    )
    private val dir = TestDir(2)
    private val dirObject = Schema.Listing.Dir(dir.uri, dir.name, dir.description, null, false)

    @Before
    fun setUp() = reset(resolver, schema, callback)

    @After
    fun tearDown() = verifyNoMoreInteractions(resolver, schema, callback)

    @Test
    fun `not resolved`() {
        with(ResolveOperation(URI1, resolver, schema, callback)) {
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
        with(ResolveOperation(file.uri, resolver, schema, callback)) {
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
        with(ResolveOperation(dir.uri, resolver, schema, callback)) {
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
        inOrder(resolver, schema, callback) {
            verify(resolver).resolve(eq(dir.uri), any())
            verify(callback).checkForCancel()
            verify(callback).onStatusChanged()
            verify(schema).resolved(dir)
        }
    }

    @Test
    fun `resolved unknown object`() {
        resolver.stub {
            on { resolve(any(), any()) } doReturn file
        }
        with(ResolveOperation(file.uri, resolver, schema, callback)) {
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
