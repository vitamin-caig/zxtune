package app.zxtune.fs.httpdir

import app.zxtune.fs.CachingCatalogTestBase
import app.zxtune.fs.dbhelpers.FileTree
import app.zxtune.fs.dbhelpers.Utils
import org.junit.After
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.ParameterizedRobolectricTestRunner

@RunWith(ParameterizedRobolectricTestRunner::class)
class CachingCatalogTest(case: TestCase) : CachingCatalogTestBase(case) {

    private val pathId = "path"

    private val path = mock<Path> {
        on { getLocalId() } doReturn pathId
    }

    private val dir1 = FileTree.Entry("dir1", "First dir", null)
    private val file1 = FileTree.Entry("file1", "First file", "32kB")
    private val dir2 = FileTree.Entry("dir", "Second dir", null)
    private val file2 = FileTree.Entry("file", "Second file", "32MB")

    private val listing = arrayListOf(dir1, file1, dir2, file2)

    private val database = mock<FileTree> {
        on { runInTransaction(any()) } doAnswer {
            it.getArgument<Utils.ThrowingRunnable>(0).run()
        }
        on { getDirLifetime(any(), any()) } doReturn lifetime
        on { find(any()) } doReturn if (case.hasCache) arrayListOf() else null
    }

    private val workingRemote = mock<RemoteCatalog> {
        on { parseDir(eq(path), any()) } doAnswer {
            it.getArgument<Catalog.DirVisitor>(1).run {
                listing.forEach { entry -> feed(entry) }
            }
        }
    }

    private val failedRemote = mock<RemoteCatalog>(defaultAnswer = { throw error })

    private val remote = if (case.isFailedRemote) failedRemote else workingRemote

    private val visitor = mock<Catalog.DirVisitor>()

    private val allMocks = arrayOf(lifetime, database, workingRemote, failedRemote, visitor)

    @After
    fun tearDown() {
        verifyNoMoreInteractions(*allMocks)
    }

    @Test
    fun `test parseDir`(): Unit = CachingCatalog(remote, database, "").let { underTest ->
        checkedQuery { underTest.parseDir(path, visitor) }

        inOrder(*allMocks).run {
            verify(database).getDirLifetime(eq(pathId), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(remote).parseDir(eq(path), any())
                if (!case.isFailedRemote) {
                    verify(database).runInTransaction(any())
                    verify(database).add(eq(pathId), eq(listing))
                    verify(lifetime).update()
                }
            }
            verify(database).find(pathId)
        }
    }

    companion object {
        @JvmStatic
        @ParameterizedRobolectricTestRunner.Parameters(name = "{0}")
        fun data() = TestCase.values().asList()
    }
}

//TODO: extract
fun Catalog.DirVisitor.feed(entry: FileTree.Entry) = if (entry.isDir) {
    acceptDir(entry.name, entry.descr)
} else {
    acceptFile(entry.name, entry.descr, entry.size)
}
