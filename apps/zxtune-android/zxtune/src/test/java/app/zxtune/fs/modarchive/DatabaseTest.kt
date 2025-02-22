package app.zxtune.fs.modarchive

import androidx.test.core.app.ApplicationProvider
import app.zxtune.fs.DatabaseTestUtils.testCheckObjectGrouping
import app.zxtune.fs.DatabaseTestUtils.testFindObjects
import app.zxtune.fs.DatabaseTestUtils.testQueryObjects
import app.zxtune.fs.DatabaseTestUtils.testVisitor
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.verify
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
            fail()
        } catch (e: IOException) {
            assertSame(error, e)
        }
        `test empty database`()
    }

    @Test
    fun `test empty database`() {
        testVisitor<Catalog.AuthorsVisitor> { visitor ->
            assertFalse(underTest.queryAuthors(visitor))
        }
        testVisitor<Catalog.GenresVisitor> { visitor ->
            assertFalse(underTest.queryGenres(visitor))
        }
        testVisitor<Catalog.TracksVisitor> { visitor ->
            underTest.queryRandomTracks(visitor)
        }
        testVisitor<Catalog.FoundTracksVisitor> { visitor ->
            underTest.findTracks("", visitor)
        }
    }

    @Test
    fun `test queryAuthors`() = testQueryObjects(
        addObject = ::addAuthor,
        queryObjects = underTest::queryAuthors,
        checkAccept = {visitor, author -> visitor.accept(author)}
    )

    @Test
    fun `test queryGenres`() = testQueryObjects(
        addObject = ::addGenre,
        queryObjects = underTest::queryGenres,
        checkAccept = {visitor, genre -> visitor.accept(genre)}
    )

    @Test
    fun `test queryTracks by author`() = testCheckObjectGrouping(
        addObject = ::addTrack,
        addGroup = ::makeAuthor,
        addObjectToGroup = underTest::addAuthorTrack,
        queryObjects = underTest::queryTracks,
        checkAccept = {visitor, track -> visitor.accept(track)}
    )

    @Test
    fun `test queryTracks by genre`() = testCheckObjectGrouping(
        addObject = ::addTrack,
        addGroup = ::makeGenre,
        addObjectToGroup = underTest::addGenreTrack,
        queryObjects = underTest::queryTracks,
        checkAccept = {visitor, track -> visitor.accept(track)}
    )

    @Test
    fun `test queryRandomTracks`() {
        val tracks = (1..10).map(::addTrack)

        testVisitor<Catalog.TracksVisitor> { visitor ->
            underTest.queryRandomTracks(visitor)
            tracks.forEach { verify(visitor).accept(it) }
        }
    }

    @Test
    fun `test findTracks`() = testFindObjects<Catalog.FoundTracksVisitor, Author, Track>(
        addObject = ::addTrack,
        addGroup = ::addAuthor,
        addObjectToGroup = underTest::addAuthorTrack,
        findAll = { visitor -> underTest.findTracks("", visitor)},
        findSpecific = {visitor -> underTest.findTracks("3", visitor)
            makeTrack(3)
        },
        checkAccept = {visitor, author, track -> visitor.accept(author, track)}
    )

    private fun addAuthor(id: Int) = makeAuthor(id).also(underTest::addAuthor)
    private fun addGenre(id: Int) = makeGenre(id).also(underTest::addGenre)
    private fun addTrack(id: Int) = makeTrack(id).also(underTest::addTrack)
}

private fun makeAuthor(id: Int) = Author(id, "author $id")
private fun makeGenre(id: Int) = Genre(id, "genre $id", id * 10)
private fun makeTrack(id: Int) = Track(id, "track$id", "Track $id", id * 1024)
