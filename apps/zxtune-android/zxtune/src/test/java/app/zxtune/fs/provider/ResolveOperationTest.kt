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
import org.mockito.kotlin.doReturnConsecutively
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
    private val fileObject = Schema.Content.File(
        file.uri, file.name, file.description, null, file.size, Schema.Content.File.Type.UNKNOWN
    )
    private val dir = TestDir(2)
    private val dirObject = Schema.Content.Dir(dir.uri, dir.name, dir.description, null, false)

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
        val fileParent = requireNotNull(file.parent)
        val parentObject = fileParent.run {
            Schema.Content.Dir(uri, name, description, null, false)
        }
        assertEquals(null, fileParent.parent)
        schema.stub {
            on { resolved(any()) } doReturnConsecutively listOf(fileObject, parentObject)
        }
        with(ResolveOperation(file.uri, resolver, schema, callback)) {
            requireNotNull(call()).run {
                assertEquals(2, count)
                moveToFirst()
                assertEquals(fileObject, Schema.Object.parse(this) as Schema.Content.File)
                moveToNext()
                assertEquals(parentObject, Schema.Object.parse(this) as Schema.Content.Dir)
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
        verify(schema).resolved(fileParent)
    }

    @Test
    fun `resolve dir with progress`() {
        resolver.stub {
            on { resolve(any(), any()) } doAnswer {
                it.getArgument<ProgressCallback>(1).onProgressUpdate(12, 34)
                dir
            }
        }
        val dirParent = requireNotNull(dir.parent)
        val dirParentObject = dirParent.run {
            Schema.Content.Dir(uri, name, description, null, false)
        }
        val dirParentParent = requireNotNull(dirParent.parent)
        val dirParentParentObject = dirParentParent.run {
            Schema.Content.Dir(uri, name, description, null, false)
        }
        assertEquals(null, dirParentParent.parent)
        schema.stub {
            on { resolved(any()) } doReturnConsecutively listOf(
                dirObject, dirParentObject, dirParentParentObject
            )
        }
        with(ResolveOperation(dir.uri, resolver, schema, callback)) {
            requireNotNull(call()).run {
                assertEquals(3, count)
                moveToFirst()
                assertEquals(dirObject, Schema.Object.parse(this) as Schema.Content.Dir)
                moveToNext()
                assertEquals(dirParentObject, Schema.Object.parse(this) as Schema.Content.Dir)
                moveToNext()
                assertEquals(dirParentParentObject, Schema.Object.parse(this) as Schema.Content.Dir)
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
            verify(schema).resolved(dirParent)
            verify(schema).resolved(dirParentParent)
        }
    }
}
