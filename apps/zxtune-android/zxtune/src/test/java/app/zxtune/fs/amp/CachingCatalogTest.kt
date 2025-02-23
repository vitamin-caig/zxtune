package app.zxtune.fs.amp

import app.zxtune.fs.CachingCatalogTestBase
import app.zxtune.fs.dbhelpers.Utils
import org.junit.After
import org.junit.Assert.assertSame
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.eq
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.verifyNoMoreInteractions
import org.mockito.kotlin.whenever
import org.robolectric.ParameterizedRobolectricTestRunner
import java.io.IOException

@RunWith(ParameterizedRobolectricTestRunner::class)
class CachingCatalogTest(case: TestCase) : CachingCatalogTestBase(case) {

    private val query = "Query"

    private val group1 = Group(1, "Group1")
    private val group2 = Group(2, "Group2")
    private val queryGroup = group1

    private val author1 = Author(11, "Author1", "First author")
    private val author2 = Author(12, "Author2", "Second author")
    private val author3 = Author(13, "Author3", "Third author")
    private val queryAuthor = author1

    private val country = Country(21, "Country")
    private val queryCountry = country

    private val track1 = Track(31, "track1", 1)
    private val track2 = Track(32, "track2", 2)

    private val database = mock<Database> {
        on { runInTransaction(any()) } doAnswer {
            it.getArgument<Utils.ThrowingRunnable>(0).run()
        }
        on { getGroupsLifetime(any()) } doReturn lifetime
        on { getAuthorsLifetime(any(), any()) } doReturn lifetime
        on { getCountryLifetime(any(), any()) } doReturn lifetime
        on { getGroupLifetime(any(), any()) } doReturn lifetime
        on { getAuthorTracksLifetime(any(), any()) } doReturn lifetime
        on { queryGroups(any()) } doReturn case.hasCache
        on { queryAuthors(any<String>(), any()) } doReturn case.hasCache
        on { queryAuthors(any<Country>(), any()) } doReturn case.hasCache
        on { queryAuthors(any<Group>(), any()) } doReturn case.hasCache
        on { queryTracks(any(), any()) } doReturn case.hasCache
    }

    private val workingRemote = mock<RemoteCatalog> {
        on { queryGroups(any()) } doAnswer {
            it.getArgument<Catalog.Visitor<Group>>(0).run {
                accept(group1)
                accept(group2)
            }
        }
        on { queryAuthors(eq(query), any()) } doAnswer {
            it.getArgument<Catalog.Visitor<Author>>(1).run {
                accept(author1)
                accept(author2)
            }
        }
        on { queryAuthors(eq(queryCountry), any()) } doAnswer {
            it.getArgument<Catalog.Visitor<Author>>(1).run {
                accept(author2)
                accept(author3)
            }
        }
        on { queryAuthors(eq(queryGroup), any()) } doAnswer {
            it.getArgument<Catalog.Visitor<Author>>(1).run {
                accept(author1)
                accept(author3)
            }
        }
        on { queryTracks(eq(queryAuthor), any()) } doAnswer {
            it.getArgument<Catalog.Visitor<Track>>(1).run {
                accept(track1)
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

    private val groupsVisitor = mock<Catalog.Visitor<Group>>()
    private val authorsVisitor = mock<Catalog.Visitor<Author>>()
    private val tracksVisitor = mock<Catalog.Visitor<Track>>()
    private val foundTracksVisitor = mock<Catalog.FoundTracksVisitor>()

    private val allMocks = arrayOf(
        lifetime,
        database,
        workingRemote,
        failedRemote,
        groupsVisitor,
        authorsVisitor,
        tracksVisitor,
        foundTracksVisitor,
    )

    @After
    fun tearDown() {
        verifyNoMoreInteractions(*allMocks)
    }

    @Test
    fun `test queryGroups`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { underTest.queryGroups(groupsVisitor) }

        inOrder(*allMocks).run {
            verify(database).getGroupsLifetime(any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryGroups(any())
                if (!case.isFailedRemote) {
                    verify(database).addGroup(group1)
                    verify(database).addGroup(group2)
                    verify(lifetime).update()
                }
            }
            verify(database).queryGroups(groupsVisitor)
        }
    }

    @Test
    fun `test queryAuthors by handle`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { underTest.queryAuthors(query, authorsVisitor) }

        inOrder(*allMocks).run {
            verify(database).getAuthorsLifetime(eq(query), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryAuthors(eq(query), any())
                if (!case.isFailedRemote) {
                    verify(database).addAuthor(author1)
                    verify(database).addAuthor(author2)
                    verify(lifetime).update()
                }
            }
            verify(database).queryAuthors(query, authorsVisitor)
        }
    }

    @Test
    fun `test queryAuthors by country`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { underTest.queryAuthors(queryCountry, authorsVisitor) }

        inOrder(*allMocks).run {
            verify(database).getCountryLifetime(eq(queryCountry), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryAuthors(eq(queryCountry), any())
                if (!case.isFailedRemote) {
                    verify(database).addAuthor(author2)
                    verify(database).addCountryAuthor(queryCountry, author2)
                    verify(database).addAuthor(author3)
                    verify(database).addCountryAuthor(queryCountry, author3)
                    verify(lifetime).update()
                }
            }
            verify(database).queryAuthors(queryCountry, authorsVisitor)
        }
    }

    @Test
    fun `test queryAuthors by group`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { underTest.queryAuthors(queryGroup, authorsVisitor) }

        inOrder(*allMocks).run {
            verify(database).getGroupLifetime(eq(queryGroup), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryAuthors(eq(queryGroup), any())
                if (!case.isFailedRemote) {
                    verify(database).addAuthor(author1)
                    verify(database).addGroupAuthor(queryGroup, author1)
                    verify(database).addAuthor(author3)
                    verify(database).addGroupAuthor(queryGroup, author3)
                    verify(lifetime).update()
                }
            }
            verify(database).queryAuthors(queryGroup, authorsVisitor)
        }
    }

    @Test
    fun `test queryTracks`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { underTest.queryTracks(queryAuthor, tracksVisitor) }

        inOrder(*allMocks).run {
            verify(database).getAuthorTracksLifetime(eq(queryAuthor), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryTracks(eq(queryAuthor), any())
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
    fun `test findTracks`(): Unit = CachingCatalog(remote, database).let { underTest ->
        try {
            underTest.findTracks(query, foundTracksVisitor)
            assert(!remoteCatalogueAvailable || !case.isFailedRemote)
        } catch (e: IOException) {
            assertSame(e, error)
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
        fun data() = TestCase.entries
    }
}
