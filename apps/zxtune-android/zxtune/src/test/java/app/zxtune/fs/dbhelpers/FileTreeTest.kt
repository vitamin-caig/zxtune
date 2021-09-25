package app.zxtune.fs.dbhelpers

import androidx.test.core.app.ApplicationProvider
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import java.io.IOException

@RunWith(RobolectricTestRunner::class)
class FileTreeTest {

    private lateinit var underTest: FileTree
    private val existingDir = "existing"
    private val unknownDir = "unknown"

    @Before
    fun setUp() {
        underTest = FileTree(ApplicationProvider.getApplicationContext(), "test")
    }

    @After
    fun tearDown() {
        underTest.close()
    }

    @Test
    fun `test empty`() {
        assertNull(underTest.find(existingDir))
        assertNull(underTest.find(unknownDir))
    }

    @Test
    fun `test content`() {
        val content = arrayListOf(
            FileTree.Entry("Dir", "some directory", null), FileTree.Entry
                ("file", "2020-02-02", "32K")
        )
        underTest.add(existingDir, content)
        assertEquals(content, underTest.find(existingDir))
        assertNull(underTest.find(unknownDir))
    }

    @Test
    fun `test runInTransaction`() {
        val error = IOException("Unused")
        try {
            underTest.runInTransaction {
                `test content`()
                throw error
            }
            fail("Unreachable")
        } catch (e: IOException) {
            assertSame(error, e)
        }
        `test empty`()
    }
}
