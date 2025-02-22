package app.zxtune.fs.zxart

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

    private val author1 = Author(1, "author1", "First author")
    private val author2 = Author(2, "author2", "Second author")

    private val queryAuthor = author1

    private val track1 = Track(10, "track1")
    private val track2 = Track(20, "track2")
    private val track3 = Track(30, "track3")

    private val party1 = Party(100, "Party1", 2000)
    private val party2 = Party(200, "Party2", 2001)

    private val queryParty = party1

    private val query = "Query"

    private val database = mock<Database> {
        on { runInTransaction(any()) } doAnswer {
            it.getArgument<Utils.ThrowingRunnable>(0).run()
        }
        on { getAuthorsLifetime(any()) } doReturn lifetime
        on { getAuthorTracksLifetime(any(), any()) } doReturn lifetime
        on { getPartiesLifetime(any()) } doReturn lifetime
        on { getPartyTracksLifetime(any(), any()) } doReturn lifetime
        on { getTopLifetime(any()) } doReturn lifetime
        on { queryAuthors(any()) } doReturn case.hasCache
        on { queryAuthorTracks(any(), any()) } doReturn case.hasCache
        on { queryParties(any()) } doReturn case.hasCache
        on { queryPartyTracks(any(), any()) } doReturn case.hasCache
        on { queryTopTracks(any(), any()) } doReturn case.hasCache
    }

    private val workingRemote = mock<RemoteCatalog> {
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
        on { queryParties(any()) } doAnswer {
            it.getArgument<Catalog.PartiesVisitor>(0).run {
                accept(party1)
                accept(party2)
            }
        }
        on { queryPartyTracks(eq(queryParty), any()) } doAnswer {
            it.getArgument<Catalog.TracksVisitor>(1).run {
                accept(track2)
                accept(track3)
            }
        }
        on { queryTopTracks(any(), any()) } doAnswer {
            it.getArgument<Catalog.TracksVisitor>(1).run {
                accept(track2)
                accept(track3)
                accept(track1)
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
    private val tracksVisitor = mock<Catalog.TracksVisitor>()
    private val partiesVisitor = mock<Catalog.PartiesVisitor>()
    private val foundTracksVisitor = mock<Catalog.FoundTracksVisitor>()

    private val allMocks = arrayOf(
        lifetime,
        database,
        failedRemote,
        workingRemote,
        progress,
        authorsVisitor,
        tracksVisitor,
        partiesVisitor,
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
            verify(database).queryAuthors(authorsVisitor)
        }
    }

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
    fun `test queryParties`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { underTest.queryParties(partiesVisitor) }

        inOrder(*allMocks).run {
            verify(database).getPartiesLifetime(any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryParties(any())
                if (!case.isFailedRemote) {
                    verify(database).addParty(party1)
                    verify(database).addParty(party2)
                    verify(lifetime).update()
                }
            }
            verify(database).queryParties(partiesVisitor)
        }
    }

    @Test
    fun `test queryPartyTracks`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { underTest.queryPartyTracks(queryParty, tracksVisitor) }

        inOrder(*allMocks).run {
            verify(database).getPartyTracksLifetime(eq(queryParty), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryPartyTracks(eq(queryParty), any())
                if (!case.isFailedRemote) {
                    verify(database).addTrack(track2)
                    verify(database).addPartyTrack(queryParty, track2)
                    verify(database).addTrack(track3)
                    verify(database).addPartyTrack(queryParty, track3)
                    verify(lifetime).update()
                }
            }
            verify(database).queryPartyTracks(queryParty, tracksVisitor)
        }
    }

    @Test
    fun `test queryTopTracks`(): Unit = CachingCatalog(remote, database).let { underTest ->
        val limit = 10

        checkedQuery { underTest.queryTopTracks(limit, tracksVisitor) }

        inOrder(*allMocks).run {
            verify(database).getTopLifetime(any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryTopTracks(eq(limit), any())
                if (!case.isFailedRemote) {
                    verify(database).addTrack(track2)
                    verify(database).addTrack(track3)
                    verify(database).addTrack(track1)
                    verify(lifetime).update()
                }
            }
            verify(database).queryTopTracks(limit, tracksVisitor)
        }
    }

    @Test
    fun `test findTracks()`(): Unit = CachingCatalog(remote, database).let { underTest ->
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

    companion object {
        @JvmStatic
        @ParameterizedRobolectricTestRunner.Parameters(name = "{0}")
        fun data() = TestCase.values().asList()
    }
}
