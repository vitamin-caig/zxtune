package app.zxtune.fs.modland

import androidx.test.core.app.ApplicationProvider
import app.zxtune.fs.DatabaseTestUtils.testCheckObjectGrouping
import app.zxtune.fs.DatabaseTestUtils.testVisitor
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.inOrder
import org.robolectric.RobolectricTestRunner
import java.io.IOException

@RunWith(RobolectricTestRunner::class)
class DatabaseTest {

    private lateinit var underTest: Database

    @Before
    fun setUp() {
        underTest = Database(ApplicationProvider.getApplicationContext())
    }

    @After
    fun tearDown() {
        underTest.close()
    }

    @Test
    fun `test runInTransaction`() {
        val error = IOException("Error")

        try {
            underTest.runInTransaction {
                `test queryTracks`()
                throw error
            }
            fail()
        } catch (e: IOException) {
            assertSame(error, e)
        }
        `test empty database`()
    }

    @Test
    fun `test empty database`() = testVisitor<Catalog.GroupsVisitor> { visitor ->
        Database.Tables.LIST.forEach {
            assertFalse(underTest.queryGroups(it, "", visitor))
        }
    }

    @Test
    fun `test queryGroups and queryGroup`() {
        val category = Database.Tables.Authors.NAME
        val nonletter = Group(1, "0", 1).also { underTest.addGroup(category, it) }
        val groups = (2..10).map { makeGroup(it) }.onEach { underTest.addGroup(category, it) }

        testVisitor<Catalog.GroupsVisitor> { visitor ->
            assertTrue(underTest.queryGroups(category, "", visitor))

            inOrder(visitor).run {
                verify(visitor).accept(nonletter)
                groups.forEach { verify(visitor).accept(it) }
            }
        }

        testVisitor<Catalog.GroupsVisitor> { visitor ->
            assertTrue(underTest.queryGroups(category, "#", visitor))

            inOrder(visitor).run {
                verify(visitor).accept(nonletter)
            }
        }

        testVisitor<Catalog.GroupsVisitor> { visitor ->
            assertTrue(underTest.queryGroups(category, "gr", visitor))

            inOrder(visitor).run {
                groups.forEach { verify(visitor).accept(it) }
            }
        }

        val queryGroup = groups[2]
        assertEquals(queryGroup, underTest.queryGroup(category, queryGroup.id))
        assertNull(underTest.queryGroup(category, 100))
    }

    @Test
    fun `test queryTracks`() {
        val category = Database.Tables.Collections.NAME
        testCheckObjectGrouping<Catalog.TracksVisitor, Group, Track>(
            addObject = ::addTrack,
            addGroup = ::makeGroup,
            addObjectToGroup = { group, track -> underTest.addGroupTrack(category, group.id, track) },
            queryObjects = { group, visitor -> underTest.queryTracks(category, group.id, visitor) },
            checkAccept = { visitor, track -> visitor.accept(track) }
        )
    }

    @Test
    fun `test findTrack`() {
        val category = Database.Tables.Formats.NAME
        val groupId = 1000
        val withSpacesAndExclamation = Track("dir1/get%20space%21", 100)
        val withBrackets = Track("dir1/dir2/%28brackets%29", 200)
        val withApostrophe = Track("dir1/dir3/dir4/track%27s", 300)

        arrayOf(withSpacesAndExclamation, withBrackets, withApostrophe).forEach {
            underTest.addTrack(it)
            underTest.addGroupTrack(category, groupId, it)
        }

        assertEquals(
            withSpacesAndExclamation,
            underTest.findTrack(category, groupId, "get space!")
        )
        assertEquals(withBrackets, underTest.findTrack(category, groupId, "(brackets)"))
        assertEquals(withApostrophe, underTest.findTrack(category, groupId, "track's"))
        assertNull(underTest.findTrack(category, groupId, "unknown"))
    }

    private fun addTrack(id: Int) = makeTrack(id).also(underTest::addTrack)
}

private fun makeGroup(id: Int) = Group(id, "group$id", id * 10)
private fun makeTrack(id: Int) = Track(id.toLong(), "track/$id", id * 1024)
