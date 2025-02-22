package app.zxtune.fs.ocremix

import androidx.room.Room
import androidx.test.core.app.ApplicationProvider
import app.zxtune.assertThrows
import org.junit.After
import org.junit.Assert
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.mock
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner
import java.io.IOException

@RunWith(RobolectricTestRunner::class)
class DatabaseTest {
    private lateinit var underTest: Database

    @Before
    fun setUp() {
        underTest = Database(
            Room.inMemoryDatabaseBuilder(
                ApplicationProvider.getApplicationContext(), DatabaseDelegate::class.java
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
        Assert.assertSame(error, assertThrows {
            underTest.runInTransaction {
                `test empty database`()
                throw error
            }
        })
        `test empty database`()
    }

    @Test
    fun `test empty database`() = with(underTest) {
        val systems = mock<Catalog.Visitor<System>> {}.apply {
            assertFalse(querySystems(this))
        }
        val organizations = mock<Catalog.Visitor<Organization>> {}.apply {
            assertFalse(queryOrganizations(this))
        }
        val games = mock<Catalog.GamesVisitor> {}.apply {
            assertFalse(queryGames(null, this))
        }
        val remixes = mock<Catalog.RemixesVisitor> {}.apply {
            assertFalse(queryRemixes(null, this))
        }
        val albums = mock<Catalog.AlbumsVisitor> {}.apply {
            assertFalse(queryAlbums(null, this))
        }
        assertArrayEquals(emptyArray(), queryMusicFiles("id"))
        assertEquals(null, queryImage("id"))
        verifyNoMoreInteractions(systems, organizations, games, remixes, albums)
    }

    @Test
    fun `test games`() = with(underTest) {
        addGame(games[0], systems[0], organizations[0])
        addGame(games[1], systems[1], organizations[0])
        addGame(games[2], systems[0], organizations[1])
        addGame(games[3], systems[2], null)
        val allSystems = mock<Catalog.Visitor<System>> {}.apply {
            assertTrue(querySystems(this))
        }.also {
            verify(it).accept(systems[0])
            verify(it).accept(systems[1])
            verify(it).accept(systems[2])
        }
        val allOrganizations = mock<Catalog.Visitor<Organization>> {}.apply {
            assertTrue(queryOrganizations(this))
        }.also {
            verify(it).accept(organizations[0])
            verify(it).accept(organizations[1])
        }
        val allGames = mock<Catalog.GamesVisitor> {}.apply {
            assertTrue(queryGames(null, this))
        }.also {
            verify(it).accept(games[0], systems[0], organizations[0])
            verify(it).accept(games[1], systems[1], organizations[0])
            verify(it).accept(games[2], systems[0], organizations[1])
            verify(it).accept(games[3], systems[2], null)
        }
        val sys0Games = mock<Catalog.GamesVisitor> {}.apply {
            assertTrue(queryGames(systems[0].asScope, this))
        }.also {
            verify(it).accept(games[0], systems[0], organizations[0])
            verify(it).accept(games[2], systems[0], organizations[1])
        }
        val sys1Games = mock<Catalog.GamesVisitor> {}.apply {
            assertTrue(queryGames(systems[1].asScope, this))
        }.also {
            verify(it).accept(games[1], systems[1], organizations[0])
        }
        val sys2Games = mock<Catalog.GamesVisitor> {}.apply {
            assertTrue(queryGames(systems[2].asScope, this))
        }.also {
            verify(it).accept(games[3], systems[2], null)
        }
        val sys3Games = mock<Catalog.GamesVisitor> {}.apply {
            assertFalse(queryGames(systems[3].asScope, this))
        }
        val org0Games = mock<Catalog.GamesVisitor> {}.apply {
            assertTrue(queryGames(organizations[0].asScope, this))
        }.also {
            verify(it).accept(games[0], systems[0], organizations[0])
            verify(it).accept(games[1], systems[1], organizations[0])
        }
        val org1Games = mock<Catalog.GamesVisitor> {}.apply {
            assertTrue(queryGames(organizations[1].asScope, this))
        }.also {
            verify(it).accept(games[2], systems[0], organizations[1])
        }
        val org2Games = mock<Catalog.GamesVisitor> {}.apply {
            assertFalse(queryGames(organizations[2].asScope, this))
        }
        verifyNoMoreInteractions(
            allSystems,
            allOrganizations,
            allGames,
            sys0Games,
            sys1Games,
            sys2Games,
            sys3Games,
            org0Games,
            org1Games,
            org2Games,
        )
    }

    @Test
    fun `test remixes`() = with(underTest) {
        addRemix(null, remixes[0], games[0])
        addRemix(null, remixes[1], games[0])
        addRemix(systems[0].asScope, remixes[2], games[1])
        addRemix(organizations[0].asScope, remixes[3], games[2])
        val allRemixes = mock<Catalog.RemixesVisitor> {}.apply {
            assertTrue(queryRemixes(null, this))
        }.also {
            verify(it).accept(remixes[0], games[0])
            verify(it).accept(remixes[1], games[0])
            verify(it).accept(remixes[2], games[1])
            verify(it).accept(remixes[3], games[2])
        }
        val game0Remixes = mock<Catalog.RemixesVisitor> {}.apply {
            assertTrue(queryRemixes(games[0].asScope, this))
        }.also {
            verify(it).accept(remixes[0], games[0])
            verify(it).accept(remixes[1], games[0])
        }
        val game1Remixes = mock<Catalog.RemixesVisitor> {}.apply {
            assertTrue(queryRemixes(games[1].asScope, this))
        }.also {
            verify(it).accept(remixes[2], games[1])
        }
        val game2Remixes = mock<Catalog.RemixesVisitor> {}.apply {
            assertTrue(queryRemixes(games[2].asScope, this))
        }.also {
            verify(it).accept(remixes[3], games[2])
        }
        val game3Remixes = mock<Catalog.RemixesVisitor> {}.apply {
            assertFalse(queryRemixes(games[3].asScope, this))
        }
        val sys0Remixes = mock<Catalog.RemixesVisitor> {}.apply {
            assertTrue(queryRemixes(systems[0].asScope, this))
        }.also {
            verify(it).accept(remixes[2], games[1])
        }
        val sys1Remixes = mock<Catalog.RemixesVisitor> {}.apply {
            assertFalse(queryRemixes(systems[1].asScope, this))
        }
        val org0Remixes = mock<Catalog.RemixesVisitor> {}.apply {
            assertTrue(queryRemixes(organizations[0].asScope, this))
        }.also {
            verify(it).accept(remixes[3], games[2])
        }
        val org1Remixes = mock<Catalog.RemixesVisitor> {}.apply {
            assertFalse(queryRemixes(organizations[1].asScope, this))
        }
        verifyNoMoreInteractions(
            allRemixes,
            game0Remixes,
            game1Remixes,
            game2Remixes,
            game3Remixes,
            sys0Remixes,
            sys1Remixes,
            org0Remixes,
            org1Remixes
        )
    }

    @Test
    fun `test albums`() = with(underTest) {
        addAlbum(null, albums[0], images[0])
        addAlbum(null, albums[0], null) // should not update
        addAlbum(games[0].asScope, albums[1], images[1])
        addAlbum(games[0].asScope, albums[2], null)
        addAlbum(systems[0].asScope, albums[1], images[2]) // should update
        addAlbum(organizations[0].asScope, albums[3], images[3])

        val allAlbums = mock<Catalog.AlbumsVisitor> {}.apply {
            assertTrue(queryAlbums(null, this))
        }.also {
            verify(it).accept(albums[0], images[0])
            verify(it).accept(albums[1], images[2])
            verify(it).accept(albums[2], null)
            verify(it).accept(albums[3], images[3])
        }
        val game0Albums = mock<Catalog.AlbumsVisitor> {}.apply {
            assertTrue(queryAlbums(games[0].asScope, this))
        }.also {
            verify(it).accept(albums[1], images[2])
            verify(it).accept(albums[2], null)
        }
        val game1Albums = mock<Catalog.AlbumsVisitor> {}.apply {
            assertFalse(queryAlbums(games[1].asScope, this))
        }
        val sys0Albums = mock<Catalog.AlbumsVisitor> {}.apply {
            assertTrue(queryAlbums(systems[0].asScope, this))
        }.also {
            verify(it).accept(albums[1], images[2])
        }
        val sys1Albums = mock<Catalog.AlbumsVisitor> {}.apply {
            assertFalse(queryAlbums(systems[1].asScope, this))
        }
        val org0Albums = mock<Catalog.AlbumsVisitor> {}.apply {
            assertTrue(queryAlbums(organizations[0].asScope, this))
        }.also {
            verify(it).accept(albums[3], images[3])
        }
        val org1Albums = mock<Catalog.AlbumsVisitor> {}.apply {
            assertFalse(queryAlbums(organizations[1].asScope, this))
        }
        verifyNoMoreInteractions(
            allAlbums, game0Albums, game1Albums, sys0Albums, sys1Albums, org0Albums, org1Albums
        )
    }

    @Test
    fun `test music files`() = with(underTest) {
        addMusicFile("id0", musics[0])
        addMusicFile("id1", musics[0], 0) // reuse path, another scope
        addMusicFile("id1", musics[1], 1)
        addMusicFile("id1", musics[2], 2)
        addMusicFile("id1", musics[2], null) // overwrite
        addMusicFile("id2", musics[2], 3) // reuse path, another scope
        assertArrayEquals(arrayOf(QueriedMusic(musics[0], null)), queryMusicFiles("id0"))
        assertArrayEquals(
            arrayOf(
                QueriedMusic(musics[0], 0),
                QueriedMusic(musics[1], 1),
                QueriedMusic(musics[2], null)
            ), queryMusicFiles("id1")
        )
        assertArrayEquals(arrayOf(QueriedMusic(musics[2], 3)), queryMusicFiles("id2"))
        assertArrayEquals(arrayOf(), queryMusicFiles("id3"))

        deleteMusicFiles("id1")
        assertArrayEquals(arrayOf(QueriedMusic(musics[0], null)), queryMusicFiles("id0"))
        assertArrayEquals(arrayOf(), queryMusicFiles("id1"))
        assertArrayEquals(arrayOf(QueriedMusic(musics[2], 3)), queryMusicFiles("id2"))
        assertArrayEquals(arrayOf(), queryMusicFiles("id3"))
    }

    @Test
    fun `test images`() = with(underTest) {
        addImage("id0", images[0])
        addImage("id0", images[1]) // overwrite
        addImage("id1", images[1]) // reuse
        addImage("id2", images[2])
        assertEquals(images[1], queryImage("id0"))
        assertEquals(images[1], queryImage("id1"))
        assertEquals(images[2], queryImage("id2"))
        assertEquals(null, queryImage("id3"))
    }

    companion object {
        private val games = Array(4) { id ->
            Game(Game.Id("game/$id"), "Game $id")
        }
        private val systems = Array(4) { id ->
            System(System.Id("sys/$id"), "System $id")
        }
        private val organizations = Array(3) { id ->
            Organization(Organization.Id("org/$id"), "Organization $id")
        }
        private val remixes = Array(4) { id ->
            Remix(Remix.Id("remix/$id"), "Remix $id")
        }
        private val albums = Array(4) { id ->
            Album(Album.Id("album/$id"), "Album $id")
        }
        private val images = Array(4) { id ->
            FilePath("image/${id}.jpg")
        }
        private val musics = Array(4) { id ->
            FilePath("music/${id}.mp3")
        }

        private val System.asScope
            get() = Catalog.SystemScope(id)

        private val Organization.asScope
            get() = Catalog.OrganizationScope(id)

        private val Game.asScope
            get() = Catalog.GameScope(id)
    }
}
