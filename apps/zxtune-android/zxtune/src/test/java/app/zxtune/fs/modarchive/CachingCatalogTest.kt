package app.zxtune.fs.modarchive

import app.zxtune.fs.CachingCatalogTestBase
import app.zxtune.fs.dbhelpers.Utils
import org.junit.After
import org.junit.Assert
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.ParameterizedRobolectricTestRunner
import java.io.IOException

@RunWith(ParameterizedRobolectricTestRunner::class)
class CachingCatalogTest(case: TestCase) : CachingCatalogTestBase(case) {

    private val query = "Query"

    private val author1 = Author(1, "author1")
    private val author2 = Author(2, "author2")

    private val queryAuthor = author1

    private val genre1 = Genre(10, "genre1", 100)
    private val genre2 = Genre(20, "genre2", 200)

    private val queryGenre = genre1

    private val track1 = Track(100, "filename1", "title1", 1000)
    private val track2 = Track(200, "filename2", "title2", 2000)
    private val track3 = Track(300, "filename3", "title3", 3000)

    private val database = mock<Database> {
        on { runInTransaction(any()) } doAnswer {
            it.getArgument<Utils.ThrowingRunnable>(0).run()
        }
        on { getAuthorsLifetime(any()) } doReturn lifetime
        on { getGenresLifetime(any()) } doReturn lifetime
        on { getAuthorTracksLifetime(any(), any()) } doReturn lifetime
        on { getGenreTracksLifetime(any(), any()) } doReturn lifetime
        on { queryAuthors(any()) } doReturn case.hasCache
        on { queryGenres(any()) } doReturn case.hasCache
        on { queryTracks(any<Author>(), any()) } doReturn case.hasCache
        on { queryTracks(any<Genre>(), any()) } doReturn case.hasCache
    }

    private val workingRemote = mock<RemoteCatalog> {
        on { queryAuthors(any(), any()) } doAnswer {
            it.getArgument<Catalog.AuthorsVisitor>(0).run {
                accept(author1)
                accept(author2)
            }
        }
        on { queryGenres(any()) } doAnswer {
            it.getArgument<Catalog.GenresVisitor>(0).run {
                accept(genre1)
                accept(genre2)
            }
        }
        on { queryTracks(eq(queryAuthor), any(), any()) } doAnswer {
            it.getArgument<Catalog.TracksVisitor>(1).run {
                accept(track1)
                accept(track2)
            }
        }
        on { queryTracks(eq(queryGenre), any(), any()) } doAnswer {
            it.getArgument<Catalog.TracksVisitor>(1).run {
                accept(track2)
                accept(track3)
            }
        }
        on { findRandomTracks(any()) } doAnswer {
            it.getArgument<Catalog.TracksVisitor>(0).run {
                accept(track3)
                accept(track2)
            }
        }
    }

    private val failedRemote = mock<RemoteCatalog>(defaultAnswer = { throw error })

    private val remote = if (case.isFailedRemote) failedRemote else workingRemote

    private val remoteCatalogueAvailable = case.isCacheExpired

    init {
        doAnswer { remoteCatalogueAvailable }.whenever(remote).searchSupported()
    }

    private val authorsVisitor = mock<Catalog.AuthorsVisitor>()
    private val genresVisitor = mock<Catalog.GenresVisitor>()
    private val tracksVisitor = mock<Catalog.TracksVisitor>()
    private val foundTracksVisitor = mock<Catalog.FoundTracksVisitor>()

    private val allMocks = arrayOf(
        lifetime,
        database,
        workingRemote,
        failedRemote,
        progress,
        authorsVisitor,
        genresVisitor,
        tracksVisitor,
        foundTracksVisitor,
    )

    @After
    fun tearDown() {
        verifyNoMoreInteractions(*allMocks)
    }

    @Test
    fun `test queryAuthors`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { underTest.queryAuthors(authorsVisitor, progress) }

        inOrder(*allMocks).run {
            verify(database).getAuthorsLifetime(any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryAuthors(any(), eq(progress))
                if (!case.isFailedRemote) {
                    verify(database).addAuthor(author1)
                    verify(database).addAuthor(author2)
                    verify(lifetime).update()
                }
            }
            verify(database).queryAuthors(authorsVisitor)
        }
    }

    @Test
    fun `test queryGenres`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { underTest.queryGenres(genresVisitor) }

        inOrder(*allMocks).run {
            verify(database).getGenresLifetime(any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryGenres(any())
                if (!case.isFailedRemote) {
                    verify(database).addGenre(genre1)
                    verify(database).addGenre(genre2)
                    verify(lifetime).update()
                }
            }
            verify(database).queryGenres(genresVisitor)
        }
    }

    @Test
    fun `test queryTracks by author`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { underTest.queryTracks(queryAuthor, tracksVisitor, progress) }

        inOrder(*allMocks).run {
            verify(database).getAuthorTracksLifetime(eq(queryAuthor), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryTracks(eq(queryAuthor), any(), eq(progress))
                if (!case.isFailedRemote) {
                    verify(database).addTrack(track1)
                    verify(database).addAuthorTrack(queryAuthor, track1)
                    verify(database).addTrack(track2)
                    verify(database).addAuthorTrack(queryAuthor, track2)
                    verify(lifetime).update()
                }
            }
            verify(database).queryTracks(queryAuthor, tracksVisitor)
        }
    }

    @Test
    fun `test queryTracks by genre`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { underTest.queryTracks(queryGenre, tracksVisitor, progress) }

        inOrder(*allMocks).run {
            verify(database).getGenreTracksLifetime(eq(queryGenre), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryTracks(eq(queryGenre), any(), eq(progress))
                if (!case.isFailedRemote) {
                    verify(database).addTrack(track2)
                    verify(database).addGenreTrack(queryGenre, track2)
                    verify(database).addTrack(track3)
                    verify(database).addGenreTrack(queryGenre, track3)
                    verify(lifetime).update()
                }
            }
            verify(database).queryTracks(queryGenre, tracksVisitor)
        }
    }

    @Test
    fun `test findTracks`(): Unit = CachingCatalog(remote, database).let { underTest ->
        try {
            underTest.findTracks(query, foundTracksVisitor)
            assert(!remoteCatalogueAvailable || !case.isFailedRemote)
        } catch (e: IOException) {
            Assert.assertSame(e, error)
        }

        inOrder(*allMocks).run {
            verify(remote).searchSupported()
            if (!remoteCatalogueAvailable) {
                verify(database).findTracks(query, foundTracksVisitor)
            } else {
                verify(remote).findTracks(query, foundTracksVisitor)
            }
        }
    }

    @Test
    fun `test findRandomTracks`(): Unit = CachingCatalog(remote, database).let { underTest ->
        try {
            underTest.findRandomTracks(tracksVisitor)
            assert(!remoteCatalogueAvailable || !case.isFailedRemote)
        } catch (e: IOException) {
            Assert.assertSame(e, error)
        }

        inOrder(*allMocks).run {
            verify(remote).searchSupported()
            if (!remoteCatalogueAvailable) {
                verify(database).queryRandomTracks(tracksVisitor)
            } else {
                verify(remote).findRandomTracks(any())
                if (!case.isFailedRemote) {
                    verify(database).addTrack(track3)
                    verify(tracksVisitor).accept(track3)
                    verify(database).addTrack(track2)
                    verify(tracksVisitor).accept(track2)
                }
            }
        }
    }

    companion object {
        @JvmStatic
        @ParameterizedRobolectricTestRunner.Parameters(name = "{0}")
        fun data() = TestCase.values().asList()
    }
}
