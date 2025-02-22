package app.zxtune.fs.amp

import androidx.test.core.app.ApplicationProvider
import app.zxtune.fs.DatabaseTestUtils.testCheckObjectGrouping
import app.zxtune.fs.DatabaseTestUtils.testFindObjects
import app.zxtune.fs.DatabaseTestUtils.testQueryObjects
import app.zxtune.fs.DatabaseTestUtils.testVisitor
import org.junit.After
import org.junit.Assert.assertFalse
import org.junit.Assert.assertSame
import org.junit.Assert.assertTrue
import org.junit.Assert.fail
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
            fail()
        } catch (e: IOException) {
            assertSame(error, e)
        }
        `test empty database`()
    }

    @Test
    fun `test empty database`() {
        testVisitor<Catalog.AuthorsVisitor> { visitor ->
            assertFalse(underTest.queryAuthors("", visitor))
        }
        testVisitor<Catalog.GroupsVisitor> { visitor ->
            assertFalse(underTest.queryGroups(visitor))
        }
        testVisitor<Catalog.FoundTracksVisitor> { visitor ->
            underTest.findTracks("", visitor)
        }
    }

    @Test
    fun `test queryAuthors by handle`() {
        val digit = addAuthor(0, '0')
        val sign = addAuthor(1, '#')
        val upper = addAuthor(2, 'Z')
        val lower = addAuthor(3, 'a')

        testVisitor<Catalog.AuthorsVisitor> { visitor ->
            assertTrue(underTest.queryAuthors("", visitor))
            inOrder(visitor).run {
                verify(visitor).accept(digit)
                verify(visitor).accept(sign)
                verify(visitor).accept(upper)
                verify(visitor).accept(lower)
            }
        }

        testVisitor<Catalog.AuthorsVisitor> { visitor ->
            assertTrue(underTest.queryAuthors(Catalog.NON_LETTER_FILTER, visitor))
            inOrder(visitor).run {
                verify(visitor).accept(digit)
                verify(visitor).accept(sign)
            }
        }

        testVisitor<Catalog.AuthorsVisitor> { visitor ->
            assertTrue(underTest.queryAuthors("A", visitor))
            inOrder(visitor).run {
                verify(visitor).accept(lower)
            }
        }

        testVisitor<Catalog.AuthorsVisitor> { visitor ->
            assertTrue(underTest.queryAuthors("Z", visitor))
            inOrder(visitor).run {
                verify(visitor).accept(upper)
            }
        }

        testVisitor<Catalog.AuthorsVisitor> { visitor ->
            assertFalse(underTest.queryAuthors("M", visitor))
        }
    }

    @Test
    fun `test queryAuthors by country`() = testCheckObjectGrouping(
        addObject = ::addAuthor,
        addGroup = ::makeCountry,
        addObjectToGroup = underTest::addCountryAuthor,
        queryObjects = underTest::queryAuthors,
        checkAccept = { visitor, author -> visitor.accept(author) }
    )

    @Test
    fun `test queryAuthors by group`() = testCheckObjectGrouping(
        addObject = ::addAuthor,
        addGroup = ::makeGroup,
        addObjectToGroup = underTest::addGroupAuthor,
        queryObjects = underTest::queryAuthors,
        checkAccept = { visitor, author -> visitor.accept(author) }
    )

    @Test
    fun `test queryTracks`() = testCheckObjectGrouping(
        addObject = ::addTrack,
        addGroup = ::makeAuthor,
        addObjectToGroup = underTest::addAuthorTrack,
        queryObjects = underTest::queryTracks,
        checkAccept = { visitor, track -> visitor.accept(track) }
    )

    @Test
    fun `test queryGroups`() = testQueryObjects(
        addObject = ::addGroup,
        queryObjects = underTest::queryGroups,
        checkAccept = { visitor, group -> visitor.accept(group) }
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
        checkAccept = { visitor, author, track -> visitor.accept(author, track) }
    )

    private fun addAuthor(id: Int) = makeAuthor(id).also(underTest::addAuthor)
    private fun addAuthor(id: Int, letter: Char) = makeAuthor(id, letter).also(underTest::addAuthor)
    private fun addGroup(id: Int) = makeGroup(id).also(underTest::addGroup)
    private fun addTrack(id: Int) = makeTrack(id).also(underTest::addTrack)
}

private fun makeAuthor(id: Int, letter: Char) = Author(id, "$letter - $id", "Author $id")
private fun makeAuthor(id: Int) = Author(id, "author $id", "Author $id")
private fun makeCountry(id: Int) = Country(id, "Country $id")
private fun makeGroup(id: Int) = Group(id, "Group $id")
private fun makeTrack(id: Int) = Track(id, "track${id}", id * 1024)
