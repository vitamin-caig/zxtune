package app.zxtune.fs.local

import android.content.ContentResolver
import android.content.UriPermission
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class PersistablePermissionsTest {

    private val resolver = mock<ContentResolver>()
    private val underTest = PersistablePermissions(resolver)

    private val idRoot1 = Identifier("root1", "")
    private val idRoot1Dir = Identifier("root1", "dir")
    private val idRoot1DirNested = Identifier("root1", "dir/nested")
    private val idRoot1DirNestedSub = Identifier("root1", "dir/nested/sub")
    private val idRoot1Some = Identifier("root1", "some")
    private val idRoot1SomeAnother = Identifier("root1", "some/another")
    private val idRoot1SomeAnotherFolder = Identifier("root1", "some/another/folder")
    private val idRoot2 = Identifier("root2", "")
    private val idRoot2Dir = Identifier("root2", "dir")

    @Before
    fun setUp() = reset(resolver)

    @Test
    fun `no permissions`() {
        idRoot1.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1Dir.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1DirNested.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1DirNestedSub.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1Some.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1SomeAnother.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1SomeAnotherFolder.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot2.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot2Dir.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
    }

    @Test
    fun `single deep permission`() {
        resolver.stub {
            on { persistedUriPermissions } doAnswer {
                arrayListOf(makeReadUriPermission(idRoot1DirNestedSub))
            }
        }
        idRoot1.let {
            assertArrayEquals(
                arrayOf(idRoot1Dir),
                underTest.getDirectChildrenOf(it).toTypedArray()
            )
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1Dir.let {
            assertArrayEquals(
                arrayOf(idRoot1DirNested),
                underTest.getDirectChildrenOf(it).toTypedArray()
            )
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1DirNested.let {
            assertArrayEquals(
                arrayOf(idRoot1DirNestedSub),
                underTest.getDirectChildrenOf(it).toTypedArray()
            )
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1DirNestedSub.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(idRoot1DirNestedSub, underTest.findAncestor(it))
        }
        idRoot1Some.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1SomeAnother.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1SomeAnotherFolder.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot2.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot2Dir.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
    }

    @Test
    fun `two medium permissions`() {
        resolver.stub {
            on { persistedUriPermissions } doAnswer {
                arrayListOf(
                    makeReadUriPermission(idRoot1DirNested),
                    makeReadUriPermission(idRoot1SomeAnother)
                )
            }
        }
        idRoot1.let {
            assertArrayEquals(
                arrayOf(idRoot1Dir, idRoot1Some),
                underTest.getDirectChildrenOf(it).toTypedArray()
            )
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1Dir.let {
            assertArrayEquals(
                arrayOf(idRoot1DirNested),
                underTest.getDirectChildrenOf(it).toTypedArray()
            )
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1DirNested.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(idRoot1DirNested, underTest.findAncestor(it))
        }
        idRoot1DirNestedSub.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(idRoot1DirNested, underTest.findAncestor(it))
        }
        idRoot1Some.let {
            assertArrayEquals(
                arrayOf(idRoot1SomeAnother),
                underTest.getDirectChildrenOf(it).toTypedArray()
            )
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1SomeAnother.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(idRoot1SomeAnother, underTest.findAncestor(it))
        }
        idRoot1SomeAnotherFolder.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(idRoot1SomeAnother, underTest.findAncestor(it))
        }
        idRoot2.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot2Dir.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
    }

    @Test
    fun `complex structure`() {
        resolver.stub {
            on { persistedUriPermissions } doAnswer {
                arrayListOf(
                    makeReadUriPermission(idRoot1Dir),
                    makeWriteUriPermission(idRoot1DirNested),
                    makeReadUriPermission(idRoot1DirNestedSub),
                    makeWriteUriPermission(idRoot1SomeAnother),
                    makeReadUriPermission(idRoot2Dir)
                )
            }
        }
        idRoot1.let {
            assertArrayEquals(
                arrayOf(idRoot1Dir),
                underTest.getDirectChildrenOf(it).toTypedArray()
            )
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1Dir.let {
            assertArrayEquals(
                arrayOf(idRoot1DirNested),
                underTest.getDirectChildrenOf(it).toTypedArray()
            )
            assertEquals(idRoot1Dir, underTest.findAncestor(it))
        }
        idRoot1DirNested.let {
            assertArrayEquals(
                arrayOf(idRoot1DirNestedSub),
                underTest.getDirectChildrenOf(it).toTypedArray()
            )
            assertEquals(idRoot1Dir, underTest.findAncestor(it))
        }
        idRoot1DirNestedSub.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(idRoot1Dir, underTest.findAncestor(it))
        }
        idRoot1Some.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1SomeAnother.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot1SomeAnotherFolder.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot2.let {
            assertArrayEquals(
                arrayOf(idRoot2Dir),
                underTest.getDirectChildrenOf(it).toTypedArray()
            )
            assertEquals(null, underTest.findAncestor(it))
        }
        idRoot2Dir.let {
            assertEquals(0, underTest.getDirectChildrenOf(it).size)
            assertEquals(idRoot2Dir, underTest.findAncestor(it))
        }
    }

    companion object {
        private fun makeReadUriPermission(id: Identifier) = mock<UriPermission> {
            on { uri } doReturn id.treeDocumentUri
            on { isReadPermission } doReturn true
        }

        private fun makeWriteUriPermission(id: Identifier) = mock<UriPermission> {
            on { uri } doReturn id.treeDocumentUri
            on { isReadPermission } doReturn false
        }
    }
}