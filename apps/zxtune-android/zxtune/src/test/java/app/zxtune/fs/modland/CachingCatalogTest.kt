package app.zxtune.fs.modland

import app.zxtune.fs.CachingCatalogTestBase
import app.zxtune.fs.dbhelpers.Utils
import org.junit.After
import org.junit.Assert
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.ParameterizedRobolectricTestRunner
import java.io.IOException

sealed interface Category {
    fun subcategory(catalogue: Catalog): Catalog.Grouping
}

object Authors : Category {
    override fun toString() = "authors"

    override fun subcategory(catalogue: Catalog) = catalogue.getAuthors()
}

object Collections : Category {
    override fun toString() = "collections"

    override fun subcategory(catalogue: Catalog) = catalogue.getCollections()
}

object Formats : Category {
    override fun toString() = "formats"

    override fun subcategory(catalogue: Catalog) = catalogue.getFormats()
}

@RunWith(ParameterizedRobolectricTestRunner::class)
class CachingCatalogTest(private val cat: Category, case: TestCase) : CachingCatalogTestBase(case) {

    private val filter = "Filter"

    private val group1 = Group(1, "group1", 10)
    private val group2 = Group(2, "group2", 20)

    private val queryGroup = group1

    private val track1 = Track("track1", 100)
    private val track2 = Track("track2", 200)

    private val queryTrack = track1

    private val database = mock<Database> {
        on { runInTransaction(any()) } doAnswer {
            it.getArgument<Utils.ThrowingRunnable>(0).run()
        }
        on { getGroupsLifetime(any(), any(), any()) } doReturn lifetime
        on { getGroupTracksLifetime(any(), any(), any()) } doReturn lifetime
        on { queryGroup(any(), eq(queryGroup.id)) } doReturn if (case.hasCache) queryGroup else null
        on {
            findTrack(any(), any(), eq(queryTrack.filename))
        } doReturn if (case.hasCache) queryTrack else null
    }

    private val workingGrouping = mock<Catalog.Grouping> {
        on { queryGroups(eq(filter), any(), any()) } doAnswer {
            it.getArgument<Catalog.Visitor<Group>>(1).run {
                accept(group1)
                accept(group2)
            }
        }
        on { getGroup(queryGroup.id) } doReturn queryGroup
        on { queryTracks(eq(queryGroup.id), any(), any()) } doAnswer {
            it.getArgument<Catalog.Visitor<Track>>(1).run {
                accept(track1)
                accept(track2)
            }
        }
    }

    private val failedGrouping = mock<Catalog.Grouping>(defaultAnswer = { throw error })

    private val grouping = if (case.isFailedRemote) failedGrouping else workingGrouping

    private val remote = mock<RemoteCatalog> {
        on { getAuthors() } doReturn grouping
        on { getCollections() } doReturn grouping
        on { getFormats() } doReturn grouping
    }

    private val groupsVisitor = mock<Catalog.Visitor<Group>>()
    private val tracksVisitor = mock<Catalog.Visitor<Track>>()

    private val allMocks = arrayOf(
        lifetime,
        database,
        remote,
        workingGrouping,
        failedGrouping,
        progress,
        groupsVisitor,
        tracksVisitor,
    )

    @After
    fun tearDown() {
        verifyNoMoreInteractions(*allMocks)
    }

    @Test
    fun `test queryGroups`(): Unit = CachingCatalog(remote, database).let { underTest ->
        checkedQuery { cat.subcategory(underTest).queryGroups(filter, groupsVisitor, progress) }

        val category = cat.toString()

        inOrder(*allMocks).run {
            // TODO: lazy
            verify(remote).getAuthors()
            verify(remote).getCollections()
            verify(remote).getFormats()

            verify(database).getGroupsLifetime(eq(category), eq(filter), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(grouping).queryGroups(eq(filter), any(), eq(progress))
                if (!case.isFailedRemote) {
                    verify(database).addGroup(category, group1)
                    verify(database).addGroup(category, group2)
                    verify(lifetime).update()
                }
            }
            verify(database).queryGroups(category, filter, groupsVisitor)
        }
    }

    @Test
    fun `test getGroup`(): Unit = CachingCatalog(remote, database).let { underTest ->
        val category = cat.toString()

        var result: Group? = null
        checkedQuery { result = cat.subcategory(underTest).getGroup(queryGroup.id) }

        if (case.hasCache || !case.isFailedRemote) {
            assertEquals(queryGroup, result)
        } else {
            assertNull(result)
        }

        inOrder(*allMocks).run {
            // TODO: lazy
            verify(remote).getAuthors()
            verify(remote).getCollections()
            verify(remote).getFormats()

            verify(database).queryGroup(category, queryGroup.id)
            if (!case.hasCache) {
                verify(grouping).getGroup(queryGroup.id)
                if (!case.isFailedRemote) {
                    verify(database).addGroup(category, queryGroup)
                }
            }
        }
    }

    @Test
    fun `test queryTracks`(): Unit = CachingCatalog(remote, database).let { underTest ->
        val category = cat.toString()

        checkedQuery {
            cat.subcategory(underTest).queryTracks(queryGroup.id, tracksVisitor, progress)
        }

        inOrder(*allMocks).run {
            // TODO: lazy
            verify(remote).getAuthors()
            verify(remote).getCollections()
            verify(remote).getFormats()

            verify(database).getGroupTracksLifetime(eq(category), eq(queryGroup.id), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(grouping).queryTracks(eq(queryGroup.id), any(), eq(progress))
                if (!case.isFailedRemote) {
                    verify(database).addTrack(track1)
                    verify(database).addGroupTrack(category, queryGroup.id, track1)
                    verify(database).addTrack(track2)
                    verify(database).addGroupTrack(category, queryGroup.id, track2)
                    verify(lifetime).update()
                }
            }
            verify(database).queryTracks(category, queryGroup.id, tracksVisitor)
        }
    }

    @Test
    fun `test getTrack`(): Unit = CachingCatalog(remote, database).let { underTest ->
        val category = cat.toString()

        if (!case.isFailedRemote) {
            // required to return valid value finally to avoid exception from method
            database.stub {
                on { queryTracks(eq(category), eq(queryGroup.id), any()) } doAnswer {
                    it.getArgument<Catalog.Visitor<Track>>(2).run {
                        accept(track1)
                        accept(track2)
                    }
                    true
                }
            }
        }

        var result: Track? = null

        // updated error
        try {
            result = cat.subcategory(underTest).getTrack(queryGroup.id, queryTrack.filename)
            assert(!case.isFailedRemote || !case.isCacheExpired || case.hasCache)
        } catch (e: IOException) {
            if (case.isFailedRemote && !case.hasCache && !case.isCacheExpired) {
                // no cached track
                // group is up-to-date but empty (see queryTracks stub)
                Assert.assertNotSame(e, error)
            } else {
                Assert.assertSame(e, error)
            }
        }

        if (case.hasCache || !case.isFailedRemote) {
            assertEquals(queryTrack, result)
        } else {
            assertNull(result)
        }

        inOrder(*allMocks).run {
            // TODO: lazy
            verify(remote).getAuthors()
            verify(remote).getCollections()
            verify(remote).getFormats()

            verify(database).findTrack(category, queryGroup.id, queryTrack.filename)
            if (!case.hasCache) {
                // as in queryTracks but with custom progress and visitor
                verify(database).getGroupTracksLifetime(eq(category), eq(queryGroup.id), any())
                verify(lifetime).isExpired
                if (case.isCacheExpired) {
                    verify(database).runInTransaction(any())
                    verify(grouping).queryTracks(eq(queryGroup.id), any(), any())
                    if (!case.isFailedRemote) {
                        verify(database).addTrack(track1)
                        verify(database).addGroupTrack(category, queryGroup.id, track1)
                        verify(database).addTrack(track2)
                        verify(database).addGroupTrack(category, queryGroup.id, track2)
                        verify(lifetime).update()
                    }
                }
                verify(database).queryTracks(eq(category), eq(queryGroup.id), any())
            }
        }
    }

    companion object {
        @JvmStatic
        @ParameterizedRobolectricTestRunner.Parameters(name = "{0}, {1}")
        fun data() = ArrayList<Array<Any>>().apply {
            for (cat in arrayOf(Authors, Collections, Formats)) {
                for (case in TestCase.values()) {
                    add(arrayOf(cat, case))
                }
            }
        }
    }
}