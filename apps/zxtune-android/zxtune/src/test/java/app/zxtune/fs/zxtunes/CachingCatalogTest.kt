package app.zxtune.fs.zxtunes

import app.zxtune.fs.CachingCatalogTestBase
import app.zxtune.fs.dbhelpers.Utils
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.anyOrNull
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.eq
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.verifyNoMoreInteractions
import org.mockito.kotlin.whenever
import org.robolectric.ParameterizedRobolectricTestRunner

@RunWith(ParameterizedRobolectricTestRunner::class)
class CachingCatalogTest(case: TestCase) : CachingCatalogTestBase(case) {

    private val query = "Query"

    private val author1 = Author(1, "author1", "First author")
    private val author2 = Author(2, "author2", "Second author")
    private val queryAuthor = author1

    private val track1 = Track(10, "track1")
    private val track2 = Track(20, "track2")

    private val database = mock<Database> {
        on { runInTransaction(any()) } doAnswer {
            it.getArgument<Utils.ThrowingRunnable>(0).run()
        }
        on { getAuthorsLifetime(any()) } doReturn lifetime
        on { getAuthorTracksLifetime(any(), any()) } doReturn lifetime
        on { queryAuthors(any(), anyOrNull()) } doReturn case.hasCache
        on { queryAuthorTracks(any(), any()) } doReturn case.hasCache
    }

    private val workingRemote = mock<Catalog> {
        on { queryAuthors(any()) } doAnswer {
            it.getArgument<Catalog.AuthorsVisitor>(0).run {
                accept(author1)
                accept(author2)
            }
        }
        on { queryAuthorTracks(eq(queryAuthor), any()) } doAnswer {
            it.getArgument<Catalog.TracksVisitor>(1).run {
                accept(track1)
                accept(track2)
            }
        }
    }

    private val failedRemote = mock<Catalog>(defaultAnswer = { throw error })

    private val remote = if (case.isFailedRemote) failedRemote else workingRemote

    private val remoteCatalogueAvailable = case.isCacheExpired

    init {
        doAnswer { remoteCatalogueAvailable }.whenever(remote).searchSupported()
    }

    private val authorsVisitor = mock<Catalog.AuthorsVisitor>()
    private val tracksVisitor = mock<Catalog.TracksVisitor>()
    private val foundTracksVisitor = mock<Catalog.FoundTracksVisitor>()

    private val allMocks = arrayOf(
        lifetime,
        database,
        failedRemote,
        workingRemote,
        progress,
        authorsVisitor,
        tracksVisitor,
        foundTracksVisitor,
    )

    @After
    fun tearDown() {
        verifyNoMoreInteractions(*allMocks)
    }

    @Test
    fun `test queryAuthors`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { underTest.queryAuthors(authorsVisitor) }

        inOrder(*allMocks).run {
            verify(database).getAuthorsLifetime(any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryAuthors(any())
                if (!case.isFailedRemote) {
                    verify(database).addAuthor(author1)
                    verify(database).addAuthor(author2)
                    verify(lifetime).update()
                }
            }
            verify(database).queryAuthors(authorsVisitor, null)
        }
    }

    @Test
    fun `test queryAuthor`(): Unit = CachingCatalog(remote, database).let { underTest ->
        val id = 123456
        checkedQuery { underTest.queryAuthor(id) }

        inOrder(*allMocks).run {
            verify(database).getAuthorsLifetime(any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryAuthors(any())
                if (!case.isFailedRemote) {
                    verify(database).addAuthor(author1)
                    verify(database).addAuthor(author2)
                    verify(lifetime).update()
                }
            }
            verify(database).queryAuthors(any(), eq(id))
        }
    }

    @Test
    fun `test queryAuthorTracks`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { underTest.queryAuthorTracks(queryAuthor, tracksVisitor) }

        inOrder(*allMocks).run {
            verify(database).getAuthorTracksLifetime(eq(queryAuthor), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryAuthorTracks(eq(queryAuthor), any())
                if (!case.isFailedRemote) {
                    verify(database).addTrack(track1)
                    verify(database).addAuthorTrack(queryAuthor, track1)
                    verify(database).addTrack(track2)
                    verify(database).addAuthorTrack(queryAuthor, track2)
                    verify(lifetime).update()
                }
            }
            verify(database).queryAuthorTracks(queryAuthor, tracksVisitor)
        }
    }

    @Test
    fun `test searchSupported`(): Unit = CachingCatalog(remote, database).let { underTest ->
        assertEquals(!remoteCatalogueAvailable, underTest.searchSupported())

        inOrder(*allMocks) {
            verify(remote).searchSupported()
        }
    }

    @Test
    fun `test findTracks`(): Unit = CachingCatalog(remote, database).let { underTest ->
        underTest.findTracks(query, foundTracksVisitor)

        inOrder(*allMocks) {
            verify(database).findTracks(query, foundTracksVisitor)
        }
    }

    companion object {
        @JvmStatic
        @ParameterizedRobolectricTestRunner.Parameters(name = "{0}")
        fun data() = TestCase.values().asList()
    }
}
