package app.zxtune.fs.provider

import android.net.Uri
import app.zxtune.fs.*
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class ListingOperationTest {

    private val uri = Uri.parse("schema://host/path?query")
    private val resolver = mock<Resolver>()
    private val schema = mock<SchemaSource> {
        on { files(any()) } doAnswer {
            it.getArgument<List<VfsFile>>(0).map { o -> makeFileObject(o) }
        }
        on { directories(any()) } doAnswer {
            it.getArgument<List<VfsDir>>(0).map { o -> makeDirObject(o) }
        }
    }
    private val callback = mock<AsyncQueryOperation.Callback>()

    private val dir1 = TestDir(1)
    private val dir2 = TestDir(2)
    private val file3 = TestFile(3, "Unused")
    private val file4 = TestFile(4, "Unused")

    @Before
    fun setUp() = clearInvocations(resolver, schema, callback)

    @After
    fun tearDown() = verifyNoMoreInteractions(resolver, schema, callback)

    @Test
    fun `not resolved`() = with(ListingOperation(uri, resolver, schema, callback)) {
        assertEquals(null, call())
        status().run {
            assertEquals(1, count)
            moveToNext()
            assertEquals(Schema.Status.Progress.createIntermediate(), Schema.Object.parse(this))
        }
        inOrder(resolver, schema) {
            verify(resolver).resolve(uri)
        }
    }

    @Test
    fun `not a dir resolved`() {
        resolver.stub {
            on { resolve(any()) } doReturn mock<VfsFile>()
        }
        with(ListingOperation(uri, resolver, schema, callback)) {
            assertEquals(null, call())
            status().run {
                assertEquals(1, count)
                moveToNext()
                assertEquals(Schema.Status.Progress.createIntermediate(), Schema.Object.parse(this))
            }
        }
        inOrder(resolver, schema) {
            verify(resolver).resolve(uri)
        }
    }

    @Test
    fun `empty dir resolved`() {
        val dir = mock<VfsDir>()
        resolver.stub {
            on { resolve(any()) } doReturn dir
        }
        with(ListingOperation(uri, resolver, schema, callback)) {
            call()!!.run {
                assertEquals(0, count)
            }
            status().run {
                assertEquals(1, count)
                moveToNext()
                assertEquals(Schema.Status.Progress.createIntermediate(), Schema.Object.parse(this))
            }
        }
        inOrder(resolver, dir, schema, callback) {
            verify(resolver).resolve(uri)
            verify(dir).enumerate(any())
            verify(callback).checkForCancel()
            verify(dir).getExtension(VfsExtensions.COMPARATOR)
            verify(schema).directories(argThat { isEmpty() })
            verify(schema).files(argThat { isEmpty() })
        }
    }

    @Test
    fun `default comparator`() {
        val dir = mock<VfsDir> {
            on { enumerate(any()) } doAnswer {
                it.getArgument<VfsDir.Visitor>(0).run {
                    onItemsCount(4)
                    onDir(dir2)
                    onFile(file4)
                    onDir(dir1)
                    onFile(file3)
                    onProgressUpdate(4, 4)
                }
            }
        }
        resolver.stub {
            on { resolve(any()) } doReturn dir
        }
        with(ListingOperation(uri, resolver, schema, callback)) {
            call()!!.run {
                assertEquals(4, count)
                moveToNext()
                assertEquals(makeDirObject(dir1), Schema.Object.parse(this))
                moveToNext()
                assertEquals(makeDirObject(dir2), Schema.Object.parse(this))
                moveToNext()
                assertEquals(makeFileObject(file3), Schema.Object.parse(this))
                moveToNext()
                assertEquals(makeFileObject(file4), Schema.Object.parse(this))
            }
            status().run {
                assertEquals(1, count)
                moveToNext()
                assertEquals(Schema.Status.Progress(4, 4), Schema.Object.parse(this))
            }
        }
        inOrder(resolver, dir, schema, callback) {
            verify(resolver).resolve(uri)
            verify(dir).enumerate(any())
            verify(callback).checkForCancel()
            verify(callback).onStatusChanged()
            verify(callback).checkForCancel()
            verify(dir).getExtension(VfsExtensions.COMPARATOR)
            verify(schema).directories(arrayListOf(dir1, dir2))
            verify(schema).files(arrayListOf(file3, file4))
        }
    }

    @Test
    fun `custom comparator`() {
        val comparator = Comparator<VfsObject> { lh, rh ->
            -(lh as TestObject).idx.compareTo((rh as TestObject).idx)
        }
        val dir = mock<VfsDir> {
            on { enumerate(any()) } doAnswer {
                it.getArgument<VfsDir.Visitor>(0).run {
                    onDir(dir1)
                    onFile(file3)
                    onDir(dir2)
                    onFile(file4)
                }
            }
            on { getExtension(VfsExtensions.COMPARATOR) } doReturn comparator
        }
        resolver.stub {
            on { resolve(any()) } doReturn dir
        }
        with(ListingOperation(uri, resolver, schema, callback)) {
            call()!!.run {
                assertEquals(4, count)
                moveToNext()
                assertEquals(makeDirObject(dir2), Schema.Object.parse(this))
                moveToNext()
                assertEquals(makeDirObject(dir1), Schema.Object.parse(this))
                moveToNext()
                assertEquals(makeFileObject(file4), Schema.Object.parse(this))
                moveToNext()
                assertEquals(makeFileObject(file3), Schema.Object.parse(this))
            }
            status().run {
                assertEquals(1, count)
                moveToNext()
                assertEquals(Schema.Status.Progress.createIntermediate(), Schema.Object.parse(this))
            }
        }
        inOrder(resolver, dir, schema, callback) {
            verify(resolver).resolve(uri)
            verify(dir).enumerate(any())
            verify(callback).checkForCancel()
            verify(dir).getExtension(VfsExtensions.COMPARATOR)
            verify(schema).directories(arrayListOf(dir2, dir1))
            verify(schema).files(arrayListOf(file4, file3))
        }
    }

    companion object {
        private fun makeDirObject(dir: VfsDir) = with(dir) {
            Schema.Listing.Dir(uri, name, description, null, false)
        }

        private fun makeFileObject(file: VfsFile) = with(file) {
            Schema.Listing.File(uri, name, description, size, null, null)
        }
    }
}
