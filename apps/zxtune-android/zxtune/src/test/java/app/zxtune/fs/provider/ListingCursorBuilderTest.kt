package app.zxtune.fs.provider

import androidx.core.net.toUri
import app.zxtune.fs.TestDir
import app.zxtune.fs.TestFile
import app.zxtune.fs.TestObject
import app.zxtune.fs.VfsObject
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.argThat
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.mock
import org.mockito.kotlin.stub
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class ListingCursorBuilderTest {

    private val dirIconUri = "icon://for/dir".toUri()
    private val file1IconUri = "icon://for/file1".toUri()
    private val file2IconUri = "icon://for/file2".toUri()

    private val dir1 = TestDir(1)
    private val dirObject1 =
        Schema.Listing.Dir(dir1.uri, dir1.name, dir1.description, dirIconUri, false)
    private val dir2 = TestDir(2)
    private val dirObject2 = Schema.Listing.Dir(dir2.uri, dir2.name, dir2.description, null, true)

    private val file1 = TestFile(10, "100")
    private val fileObject1 = Schema.Listing.File(
        file1.uri,
        file1.name,
        file1.description,
        file1IconUri,
        file1.size,
        Schema.Listing.File.Type.REMOTE
    )
    private val file2 = TestFile(20, "Cached")
    private val fileObject2 = Schema.Listing.File(
        file2.uri,
        file2.name,
        file2.description,
        file2IconUri,
        file2.size,
        Schema.Listing.File.Type.UNSUPPORTED
    )
    private val file3 = TestFile(30, "Invalid cache")
    private val fileObject3 = Schema.Listing.File(
        file3.uri, file3.name, file3.description, null, file3.size, Schema.Listing.File.Type.TRACK
    )

    private val schema = mock<SchemaSource>()

    @After
    fun tearDown() = verifyNoMoreInteractions(schema)

    @Test
    fun `empty dir`() {
        with(ListingCursorBuilder()) {
            sort().getResult(schema).run {
                assertEquals(0, count)
            }
        }
        verify(schema).directories(argThat { isEmpty() })
        verify(schema).files(argThat { isEmpty() })
    }

    @Test
    fun `progress update`() = with(ListingCursorBuilder()) {
        status.run {
            assertEquals(1, count)
            moveToFirst()
            assertEquals(
                Schema.Status.Progress.createIntermediate(),
                Schema.Object.parse(this) as Schema.Status.Progress
            )
        }
        setProgress(12, 34)
        status.run {
            assertEquals(1, count)
            moveToFirst()
            assertEquals(
                Schema.Status.Progress(12, 34), Schema.Object.parse(this) as Schema.Status.Progress
            )
        }
    }

    @Test
    fun `dirs and files`() {
        schema.stub {
            on { directories(any()) } doReturn listOf(dirObject1, dirObject2)
            on { files(any()) } doReturn listOf(fileObject1, fileObject2, fileObject3)
        }
        val comparator = Comparator<VfsObject> { lh, rh ->
            (lh as TestObject).idx.compareTo((rh as TestObject).idx)
        }
        ListingCursorBuilder().apply {
            addDir(dir2)
            addFile(file2)
            addFile(file1)
            addDir(dir1)
            addFile(file3)
        }.sort(comparator).getResult(schema).run {
            assertEquals(5, count)
            moveToNext()
            assertEquals(dirObject1, Schema.Object.parse(this) as Schema.Listing.Dir)
            moveToNext()
            assertEquals(dirObject2, Schema.Object.parse(this) as Schema.Listing.Dir)
            moveToNext()
            assertEquals(fileObject1, Schema.Object.parse(this) as Schema.Listing.File)
            moveToNext()
            assertEquals(fileObject2, Schema.Object.parse(this) as Schema.Listing.File)
            moveToNext()
            assertEquals(fileObject3, Schema.Object.parse(this) as Schema.Listing.File)
        }
        verify(schema).directories(listOf(dir1, dir2))
        verify(schema).files(listOf(file1, file2, file3))
    }
}
