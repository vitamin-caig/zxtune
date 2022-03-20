package app.zxtune.fs.provider

import android.net.Uri
import app.zxtune.fs.TestDir
import app.zxtune.fs.VfsDir
import app.zxtune.fs.VfsObject
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class ParentsCursorBuilderTest {

    private val dir = object : TestDir(1) {
        override val parent: VfsObject? = null
    }
    private val dirObject = Schema.Parents.Object(dir.uri, "root", null)

    private val nested = object : TestDir(2) {
        override val parent: VfsObject = dir
    }
    private val nestedObject = Schema.Parents.Object(nested.uri, "nested", 2)

    private val schema = mock<SchemaSource>()

    @Before
    fun setUp() = reset(schema)

    @After
    fun tearDown() = verifyNoMoreInteractions(schema)

    @Test
    fun `test root`() {
        val root = mock<VfsDir> {
            on { uri } doReturn Uri.EMPTY
        }
        with(ParentsCursorBuilder.makeParents(root, schema)) {
            assertEquals(0, count)
        }
        verify(schema).parents(arrayListOf())
    }

    @Test
    fun `test single dir`() {
        schema.stub {
            on { parents(any()) } doReturn arrayListOf(dirObject)
        }
        with(ParentsCursorBuilder.makeParents(dir, schema)) {
            assertEquals(1, count)
            moveToFirst()
            assertEquals(dirObject, Schema.Parents.Object.parse(this))
        }
        verify(schema).parents(argThat { size == 1 && get(0) === dir })
    }

    @Test
    fun `test dirs chain`() {
        schema.stub {
            on { parents(any()) } doReturn arrayListOf(dirObject, nestedObject)
        }
        with(ParentsCursorBuilder.makeParents(nested, schema)) {
            assertEquals(2, count)
            moveToFirst()
            assertEquals(dirObject, Schema.Parents.Object.parse(this))
            moveToNext()
            assertEquals(nestedObject, Schema.Parents.Object.parse(this))
        }
        verify(schema).parents(argThat { size == 2 && get(0) === dir && get(1) === nested })
    }
}
