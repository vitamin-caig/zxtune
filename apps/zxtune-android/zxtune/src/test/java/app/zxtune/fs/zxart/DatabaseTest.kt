package app.zxtune.fs.zxart

import androidx.test.core.app.ApplicationProvider
import app.zxtune.fs.DatabaseTestUtils.testCheckObjectGrouping
import app.zxtune.fs.DatabaseTestUtils.testFindObjects
import app.zxtune.fs.DatabaseTestUtils.testQueryObjects
import app.zxtune.fs.DatabaseTestUtils.testVisitor
import org.junit.After
import org.junit.Assert
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
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
                `test findTracks`()
                throw error
            }
            Assert.fail()
        } catch (e: IOException) {
            Assert.assertSame(error, e)
        }
        `test empty database`()
    }

    @Test
    fun `test empty database`() {
        testVisitor<Catalog.AuthorsVisitor> { visitor ->
            assertFalse(underTest.queryAuthors(visitor))
        }
        testVisitor<Catalog.PartiesVisitor> { visitor ->
            assertFalse(underTest.queryParties(visitor))
        }
        testVisitor<Catalog.TracksVisitor> { visitor ->
            assertFalse(underTest.queryTopTracks(100, visitor))
        }
        testVisitor<Catalog.FoundTracksVisitor> { visitor ->
            underTest.findTracks("", visitor)
        }
    }

    @Test
    fun `test queryAuthors`() = testQueryObjects(
        addObject = ::addAuthor,
        queryObjects = underTest::queryAuthors,
        checkAccept = { visitor, author -> visitor.accept(author) }
    )

    @Test
    fun `test queryParties`() = testQueryObjects(
        addObject = ::addParty,
        queryObjects = underTest::queryParties,
        checkAccept = { visitor, party -> visitor.accept(party) }
    )

    @Test
    fun `test queryAuthorTracks`() = testCheckObjectGrouping(
        addObject = ::addTrack,
        addGroup = ::makeAuthor,
        addObjectToGroup = underTest::addAuthorTrack,
        queryObjects = underTest::queryAuthorTracks,
        checkAccept = { visitor, track -> visitor.accept(track) }
    )

    @Test
    fun `test queryPartyTracks`() = testCheckObjectGrouping(
        addObject = ::addTrack,
        addGroup = ::makeParty,
        addObjectToGroup = underTest::addPartyTrack,
        queryObjects = underTest::queryPartyTracks,
        checkAccept = { visitor, track -> visitor.accept(track) }
    )

    @Test
    fun `test queryTopTracks`() {
        val tracks =
            (1..5).map { id -> Track(id, "track$id", "", id.toDouble().toString(), "", 0, "", 0) }
                .onEach { underTest.addTrack(it) }

        testVisitor<Catalog.TracksVisitor> { visitor ->
            assertTrue(underTest.queryTopTracks(3, visitor))
            inOrder(visitor).run {
                verify(visitor).accept(tracks[4])
                verify(visitor).accept(tracks[3])
                verify(visitor).accept(tracks[2])
            }
        }
    }

    @Test
    fun `test findTracks`() = testFindObjects<Catalog.FoundTracksVisitor, Author, Track>(
        addObject = ::addTrack,
        addGroup = ::addAuthor,
        addObjectToGroup = underTest::addAuthorTrack,
        findAll = { visitor -> underTest.findTracks("", visitor) },
        findSpecific = { visitor ->
            underTest.findTracks("3", visitor)
            makeTrack(3)
        },
        checkAccept = { visitor, author, track -> visitor.accept(author, track) }
    )

    private fun addAuthor(id: Int) = makeAuthor(id).also(underTest::addAuthor)
    private fun addParty(id: Int) = makeParty(id).also(underTest::addParty)
    private fun addTrack(id: Int) = makeTrack(id).also(underTest::addTrack)
}

private fun makeAuthor(id: Int) = Author(id, "author$id", "Author $id")
private fun makeParty(id: Int) = Party(id, "party$id", 2000 + id)
private fun makeTrack(id: Int) = Track(id, "track$id")
