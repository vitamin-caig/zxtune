package app.zxtune.fs.zxtunes

import androidx.test.core.app.ApplicationProvider
import app.zxtune.fs.DatabaseTestUtils.testCheckObjectGrouping
import app.zxtune.fs.DatabaseTestUtils.testFindObjects
import app.zxtune.fs.DatabaseTestUtils.testQueryObjects
import app.zxtune.fs.DatabaseTestUtils.testVisitor
import org.junit.After
import org.junit.Assert
import org.junit.Assert.assertFalse
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
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
        testVisitor<Catalog.FoundTracksVisitor> { visitor ->
            underTest.findTracks("", visitor)
        }
    }

    @Test
    fun `test queryAuthors`() = testQueryObjects(
        addObject = ::addAuthor,
        queryObjects = underTest::queryAuthors,
        checkCountHint = { visitor, hint -> visitor.setCountHint(hint) },
        checkAccept = { visitor, author -> visitor.accept(author) }
    )

    @Test
    fun `test queryAuthorTracks`() = testCheckObjectGrouping(
        addObject = ::addTrack,
        addGroup = ::makeAuthor,
        addObjectToGroup = underTest::addAuthorTrack,
        queryObjects = underTest::queryAuthorTracks,
        checkCountHint = { visitor, hint -> visitor.setCountHint(hint) },
        checkAccept = { visitor, track -> visitor.accept(track) }
    )

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
        checkCountHint = { visitor, hint -> visitor.setCountHint(hint) },
        checkAccept = { visitor, author, track -> visitor.accept(author, track) }
    )

    private fun addAuthor(id: Int) = makeAuthor(id).also(underTest::addAuthor)
    private fun addTrack(id: Int) = makeTrack(id).also(underTest::addTrack)
}

private fun makeAuthor(id: Int) = Author(id, "author$id", hasPhoto = 0 == id % 2)
private fun makeTrack(id: Int) = Track(id, "track$id", duration = 0, date = 0)

