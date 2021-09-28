package app.zxtune.fs.vgmrips

import androidx.room.Room
import androidx.test.core.app.ApplicationProvider
import app.zxtune.TimeStamp
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
        underTest = Database(
            Room.inMemoryDatabaseBuilder(
                ApplicationProvider.getApplicationContext(),
                Database.DatabaseDelegate::class.java
            ).allowMainThreadQueries().build()
        )
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
                `test queryGroups`()
                throw error
            }
            fail()
        } catch (e: IOException) {
            assertSame(error, e)
        }
        `test empty database`()
    }

    @Test
    fun `test empty database`() {
        testVisitor<Catalog.Visitor<Group>> { visitor ->
            arrayOf(
                Database.TYPE_CHIP,
                Database.TYPE_COMPANY,
                Database.TYPE_COMPOSER,
                Database.TYPE_SYSTEM
            ).forEach { type ->
                assertFalse(underTest.queryGroups(type, visitor))
            }
        }
        assertNull(underTest.queryRandomPack())
    }

    @Test
    fun `test queryGroups`() {
        val groups = (1..10).map { makeGroup(it) }
            .onEachIndexed { idx, group -> underTest.addGroup(idx % 4, group) }
        testVisitor<Catalog.Visitor<Group>> { visitor ->
            arrayOf(
                Database.TYPE_CHIP, //2
                Database.TYPE_COMPANY,//0
                Database.TYPE_COMPOSER,//1
                Database.TYPE_SYSTEM//3
            ).forEach { type ->
                assertTrue(underTest.queryGroups(type, visitor))
            }

            inOrder(visitor).run {
                verify(visitor).accept(groups[2])
                verify(visitor).accept(groups[6])

                verify(visitor).accept(groups[0])
                verify(visitor).accept(groups[4])
                verify(visitor).accept(groups[8])

                // sorted
                verify(visitor).accept(groups[9])
                verify(visitor).accept(groups[1])
                verify(visitor).accept(groups[5])

                verify(visitor).accept(groups[3])
                verify(visitor).accept(groups[7])
            }
        }
    }

    @Test
    fun `test queryGroupPacks`() = testCheckObjectGrouping<Catalog.Visitor<Pack>, Group, Pack>(
        addObject = ::addPack,
        addGroup = ::makeGroup,
        addObjectToGroup = { group, pack -> underTest.addGroupPack(group.id, pack) },
        queryObjects = { group, visitor -> underTest.queryGroupPacks(group.id, visitor) },
        checkCountHint = null,
        checkAccept = { visitor, pack -> visitor.accept(pack) }
    )

    @Test
    fun `test queryPack`() {
        val packs = (1..10).map(::addPack)

        packs.forEach { assertEquals(it, underTest.queryPack(it.id)) }
        assertNull(underTest.queryPack("0"))
    }

    @Test
    fun `test queryRandomPack`() {
        addPack(1)

        assertNull(underTest.queryRandomPack())

        val hasTracks1 = addPack(2).also {
            underTest.addPackTrack(it.id, makeTrack(1))
        }

        assertEquals(hasTracks1, underTest.queryRandomPack())
        assertEquals(hasTracks1, underTest.queryRandomPack())

        val withTracks =
            (3..10).map(::addPack).onEach {
                underTest.addPackTrack(it.id, makeTrack(1))
            }

        while (true) {
            val res = underTest.queryRandomPack()
            if (res == hasTracks1) {
                continue
            }
            assertTrue(withTracks.contains(res))
            break
        }
    }

    @Test
    fun `test queryPackTracks`() {
        val pack = makePack(100)

        testVisitor<Catalog.Visitor<Track>> { visitor ->
            assertFalse(underTest.queryPackTracks(pack.id, visitor))
        }

        val tracks = (1..5).map(::makeTrack).onEach { underTest.addPackTrack(pack.id, it) }

        testVisitor<Catalog.Visitor<Track>> { visitor ->
            assertTrue(underTest.queryPackTracks(pack.id, visitor))

            inOrder(visitor).run {
                tracks.forEach { verify(visitor).accept(it) }
            }
        }
    }

    private fun addPack(id: Int) = makePack(id).also(underTest::addPack)
}

private fun makeGroup(id: Int) = Group(id.toString(), "Group $id", id * 3)
private fun makePack(id: Int) = Pack(id.toString(), "Pack $id")
private fun makeTrack(id: Int) =
    Track(id, "Track $id", TimeStamp.fromSeconds(id.toLong()), "track${id}")
