package app.zxtune.fs.vgmrips

import app.zxtune.TimeStamp
import app.zxtune.fs.CachingCatalogTestBase
import app.zxtune.fs.dbhelpers.Utils
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*
import org.robolectric.ParameterizedRobolectricTestRunner

private sealed interface Category {
    val type: Int
    fun subcategory(cat: Catalog): Catalog.Grouping
}

private object Chips : Category {
    override val type: Int
        get() = Database.TYPE_CHIP

    override fun toString() = "chip"
    override fun subcategory(cat: Catalog) = cat.chips()
}

private object Companies : Category {
    override val type: Int
        get() = Database.TYPE_COMPANY

    override fun toString() = "company"
    override fun subcategory(cat: Catalog) = cat.companies()
}

private object Composers : Category {
    override val type: Int
        get() = Database.TYPE_COMPOSER

    override fun toString() = "composer"
    override fun subcategory(cat: Catalog) = cat.composers()
}

private object Systems : Category {
    override val type: Int
        get() = Database.TYPE_SYSTEM

    override fun toString() = "system"
    override fun subcategory(cat: Catalog) = cat.systems()
}

@RunWith(ParameterizedRobolectricTestRunner::class)
class CachingCatalogTest(case: TestCase) : CachingCatalogTestBase(case) {

    private val group1 = Group("1", "group1", 10)
    private val group2 = Group("2", "group2", 20)
    private val group3 = Group("3", "group3", 30)

    private val queryGroup = group1
    private val unknownGroup = group3

    private val pack1 = Pack("100", "Pack 1")
    private val pack2 = Pack("200", "Pack 2")
    private val pack3 = Pack("300", "Pack 3")

    private val queryPack = pack1
    private val randomPack = pack2
    private val unknownPack = pack3

    private val track1 = Track(1000, "Track1", TimeStamp.fromSeconds(10), "location1")
    private val track2 = Track(2000, "Track2", TimeStamp.fromSeconds(20), "location2")

    private val database = mock<Database> {
        on { runInTransaction(any()) } doAnswer {
            it.getArgument<Utils.ThrowingRunnable>(0).run()
        }
        on { getLifetime(any(), any()) } doReturn lifetime
        on { queryGroups(any(), any()) } doReturn case.hasCache
        on { queryGroupPacks(any(), any()) } doReturn case.hasCache
        on { queryPack(queryPack.id) } doReturn if (case.hasCache) queryPack else null
        on { queryRandomPack() } doReturn if (case.hasCache) randomPack else null
    }

    private val workingGrouping = mock<Catalog.Grouping> {
        on { query(any()) } doAnswer {
            it.getArgument<Catalog.Visitor<Group>>(0).run {
                accept(group1)
                accept(group2)
            }
        }
        on { queryPacks(eq(queryGroup.id), any(), any()) } doAnswer {
            it.getArgument<Catalog.Visitor<Pack>>(1).run {
                accept(pack1)
                accept(pack2)
            }
        }
    }

    private val workingRemote = mock<RemoteCatalog> {
        on { findPack(eq(queryPack.id), any()) } doAnswer {
            it.getArgument<Catalog.Visitor<Track>>(1).run {
                accept(track1)
                accept(track2)
            }
            queryPack
        }
        on { findRandomPack(any()) } doAnswer {
            it.getArgument<Catalog.Visitor<Track>>(0).run {
                accept(track2)
                accept(track1)
            }
            randomPack
        }
    }

    private val failedGrouping = mock<Catalog.Grouping>(defaultAnswer = { throw error })
    private val failedRemote = mock<RemoteCatalog>(defaultAnswer = { throw error })

    private val remote = if (case.isFailedRemote) failedRemote else workingRemote
    private val grouping = if (case.isFailedRemote) failedGrouping else workingGrouping

    private val remoteCatalogueAvailable = case.isCacheExpired

    init {
        doAnswer { remoteCatalogueAvailable }.whenever(remote).isAvailable()
        doAnswer { grouping }.whenever(remote).chips()
        doAnswer { grouping }.whenever(remote).companies()
        doAnswer { grouping }.whenever(remote).composers()
        doAnswer { grouping }.whenever(remote).systems()
    }

    private val groupsVisitor = mock<Catalog.Visitor<Group>>()
    private val packsVisitor = mock<Catalog.Visitor<Pack>>()
    private val tracksVisitor = mock<Catalog.Visitor<Track>>()

    private val allMocks = arrayOf(
        lifetime,
        database,
        failedRemote,
        workingRemote,
        failedGrouping,
        workingGrouping,
        progress,
        groupsVisitor,
        packsVisitor,
        tracksVisitor,
    )

    @After
    fun tearDown() {
        verifyNoMoreInteractions(*allMocks)
    }

    @Test
    fun `test query`(): Unit = CachingCatalog(remote, database).let { underTest ->
        for (cat in arrayOf(Chips, Companies, Composers, Systems)) {
            clearInvocations(*allMocks)
            checkedQuery { cat.subcategory(underTest).query(groupsVisitor) }

            inOrder(*allMocks).run {
                cat.subcategory(verify(remote))
                verify(database).getLifetime(eq(cat.toString()), any())
                verify(lifetime).isExpired
                if (case.isCacheExpired) {
                    verify(database).runInTransaction(any())
                    verify(grouping).query(any())
                    if (!case.isFailedRemote) {
                        verify(database).addGroup(cat.type, group1)
                        verify(database).addGroup(cat.type, group2)
                        verify(lifetime).update()
                    }
                }
                verify(database).queryGroups(cat.type, groupsVisitor)
            }
            verifyNoMoreInteractions(*allMocks)
        }
    }

    @Test
    fun `test queryPacks`(): Unit = CachingCatalog(remote, database).let { underTest ->
        for (cat in arrayOf(Chips, Companies, Composers, Systems)) {
            clearInvocations(*allMocks)
            checkedQuery {
                cat.subcategory(underTest).queryPacks(queryGroup.id, packsVisitor, progress)
            }

            inOrder(*allMocks).run {
                cat.subcategory(verify(remote))
                verify(database).getLifetime(eq(queryGroup.id), any())
                verify(lifetime).isExpired
                if (case.isCacheExpired) {
                    verify(database).runInTransaction(any())
                    verify(grouping).queryPacks(eq(queryGroup.id), any(), eq(progress))
                    if (!case.isFailedRemote) {
                        verify(database).addGroupPack(queryGroup.id, pack1)
                        verify(database).addGroupPack(queryGroup.id, pack2)
                        verify(lifetime).update()
                    }
                }
                verify(database).queryGroupPacks(queryGroup.id, packsVisitor)
            }
            verifyNoMoreInteractions(*allMocks)
        }
    }

    @Test
    fun `test queryPacks unknown`(): Unit = CachingCatalog(remote, database).let { underTest ->
        for (cat in arrayOf(Chips, Companies, Composers, Systems)) {
            clearInvocations(*allMocks)
            checkedQuery {
                cat.subcategory(underTest).queryPacks(unknownGroup.id, packsVisitor, progress)
            }

            inOrder(*allMocks).run {
                cat.subcategory(verify(remote))
                verify(database).getLifetime(eq(unknownGroup.id), any())
                verify(lifetime).isExpired
                if (case.isCacheExpired) {
                    verify(database).runInTransaction(any())
                    verify(grouping).queryPacks(eq(unknownGroup.id), any(), eq(progress))
                    if (!case.isFailedRemote) {
                        verify(lifetime).update()
                    }
                }
                verify(database).queryGroupPacks(unknownGroup.id, packsVisitor)
            }
            verifyNoMoreInteractions(*allMocks)
        }
    }

    @Test
    fun `test findPack`(): Unit = CachingCatalog(remote, database).let { underTest ->
        var result: Pack? = null
        checkedQuery { result = underTest.findPack(queryPack.id, tracksVisitor) }

        if (case.hasCache && !(case.isCacheExpired && case.isFailedRemote)) {
            assertEquals(queryPack, result)
        } else {
            assertNull(result)
        }

        inOrder(*allMocks).run {
            verify(database).getLifetime(eq(queryPack.id), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).findPack(eq(queryPack.id), any())
                if (!case.isFailedRemote) {
                    verify(database).addPackTrack(queryPack.id, track1)
                    verify(database).addPackTrack(queryPack.id, track2)
                    verify(database).addPack(queryPack)
                    verify(lifetime).update()
                }
            }
            verify(database).queryPack(queryPack.id)
            if (case.hasCache) {
                verify(database).queryPackTracks(queryPack.id, tracksVisitor)
            }
        }
    }

    @Test
    fun `test findPack unknown`(): Unit = CachingCatalog(remote, database).let { underTest ->
        var result: Pack? = null
        checkedQuery { result = underTest.findPack(unknownPack.id, tracksVisitor) }

        assertNull(result)

        inOrder(*allMocks).run {
            verify(database).getLifetime(eq(unknownPack.id), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).findPack(eq(unknownPack.id), any())
                if (!case.isFailedRemote) {
                    verify(lifetime).update()
                }
            }
            verify(database).queryPack(unknownPack.id)
        }
    }

    @Test
    fun `test findRandomPack`(): Unit = CachingCatalog(remote, database).let { underTest ->
        var result: Pack? = null
        checkedQuery { result = underTest.findRandomPack(tracksVisitor) }

        if ((remoteCatalogueAvailable && !case.isFailedRemote) || (!remoteCatalogueAvailable && case.hasCache)) {
            assertEquals(randomPack, result)
        } else {
            assertNull(result)
        }

        inOrder(*allMocks).run {
            verify(remote).isAvailable()
            if (remoteCatalogueAvailable) {
                verify(database).runInTransaction(any())
                verify(remote).findRandomPack(any())
                if (!case.isFailedRemote) {
                    verify(database).addPack(randomPack)
                    verify(database).addPackTrack(randomPack.id, track2)
                    verify(tracksVisitor).accept(track2)
                    verify(database).addPackTrack(randomPack.id, track1)
                    verify(tracksVisitor).accept(track1)
                }
            } else {
                verify(database).queryRandomPack()
                if (case.hasCache) {
                    verify(database).queryPackTracks(randomPack.id, tracksVisitor)
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
