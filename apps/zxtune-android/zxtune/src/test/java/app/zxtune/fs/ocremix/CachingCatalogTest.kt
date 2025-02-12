package app.zxtune.fs.ocremix

import app.zxtune.fs.CachingCatalogTestBase
import app.zxtune.fs.dbhelpers.Utils
import org.junit.After
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.anyOrNull
import org.mockito.kotlin.clearInvocations
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.eq
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.reset
import org.mockito.kotlin.stub
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.ParameterizedRobolectricTestRunner

@RunWith(ParameterizedRobolectricTestRunner::class)
class CachingCatalogTest(case: TestCase) : CachingCatalogTestBase(case) {
    private val database = mock<Database> {
        on { runInTransaction(any()) } doAnswer {
            it.getArgument<Utils.ThrowingRunnable>(0).run()
        }
        on { getLifetime(any(), any()) } doReturn lifetime
        on { querySystems(any()) } doReturn case.hasCache
        on { queryOrganizations(any()) } doReturn case.hasCache
        on { queryGames(anyOrNull(), any()) } doReturn case.hasCache
        on { queryRemixes(anyOrNull(), any()) } doReturn case.hasCache
        on { queryAlbums(anyOrNull(), any()) } doReturn case.hasCache
        on { queryMusicFiles(any()) } doReturn if (case.hasCache) musics else emptyArray()
        on { queryImage(any()) } doReturn if (case.hasCache) images[0] else null
    }

    private val workingRemote = mock<RemoteCatalog> {
        on { querySystems(any()) } doAnswer {
            with(it.getArgument<Catalog.Visitor<System>>(0)) {
                setCountHint(systems.size)
                systems.forEach(this::accept)
            }
        }
        on { queryOrganizations(any(), any()) } doAnswer {
            with(it.getArgument<Catalog.Visitor<Organization>>(0)) {
                setCountHint(organizations.size)
                organizations.forEach(this::accept)
            }
        }
        on { queryGames(anyOrNull(), any(), any()) } doAnswer {
            with(it.getArgument<Catalog.GamesVisitor>(1)) {
                if (it.getArgument<Catalog.Scope>(0) == null) {
                    setCountHint(4)
                    accept(games[0], systems[0], organizations[0])
                    accept(games[1], systems[1], organizations[0])
                    accept(games[2], systems[0], organizations[1])
                    accept(games[3], systems[2], null)
                } else {
                    setCountHint(2)
                    accept(games[0], systems[0], organizations[0])
                    accept(games[2], systems[0], organizations[1])
                }
            }
        }
        on { queryRemixes(anyOrNull(), any(), any()) } doAnswer {
            with(it.getArgument<Catalog.RemixesVisitor>(1)) {
                if (it.getArgument<Catalog.Scope>(0) == null) {
                    setCountHint(4)
                    accept(remixes[0], games[0])
                    accept(remixes[1], games[0])
                    accept(remixes[2], games[1])
                    accept(remixes[3], games[2])
                } else {
                    setCountHint(2)
                    accept(remixes[0], games[0])
                    accept(remixes[1], games[0])
                }
            }
        }
        on { queryAlbums(anyOrNull(), any(), any()) } doAnswer {
            with(it.getArgument<Catalog.AlbumsVisitor>(1)) {
                if (it.getArgument<Catalog.Scope>(0) == null) {
                    setCountHint(4)
                    accept(albums[0], images[0])
                    accept(albums[1], images[2])
                    accept(albums[2], null)
                    accept(albums[3], images[3])
                } else {
                    setCountHint(2)
                    accept(albums[1], images[2])
                    accept(albums[2], null)
                }
            }
        }
        on { findRemixPath(any()) } doReturn musics[0].path
        on { queryAlbumTracks(any(), any()) } doAnswer {
            with(it.getArgument<Catalog.AlbumTracksVisitor>(1)) {
                musics.forEachIndexed { idx, file ->
                    if (idx > 0) {
                        accept(file.path, requireNotNull(file.size))
                    }
                }
            }
        }
        on { queryAlbumImage(any()) } doReturn images[1]
    }
    private val failedRemote = mock<RemoteCatalog>(defaultAnswer = { throw error })
    private val remote = if (case.isFailedRemote) failedRemote else workingRemote
    private val systemsVisitor = mock<Catalog.Visitor<System>>()
    private val organizationsVisitor = mock<Catalog.Visitor<Organization>>()
    private val gamesVisitor = mock<Catalog.GamesVisitor>()
    private val remixesVisitor = mock<Catalog.RemixesVisitor>()
    private val albumsVisitor = mock<Catalog.AlbumsVisitor>()
    private val albumTracksVisitor = mock<Catalog.AlbumTracksVisitor>()

    private val allMocks = arrayOf(
        lifetime,
        database,
        failedRemote,
        workingRemote,
        progress,
        systemsVisitor,
        organizationsVisitor,
        gamesVisitor,
        remixesVisitor,
        albumsVisitor,
        albumTracksVisitor,
    )

    @After
    fun tearDown() = verifyNoMoreInteractions(*allMocks)

    @Test
    fun `test querySystems`(): Unit = with(CachingCatalog(remote, database)) {
        checkedQuery { querySystems(systemsVisitor) }

        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("systems"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).querySystems(any())
                if (!case.isFailedRemote) {
                    systems.forEach { sys ->
                        verify(database).addSystem(sys)
                    }
                    verify(lifetime).update()
                }
            }
            verify(database).querySystems(systemsVisitor)
        }
    }

    @Test
    fun `test queryOrganizations`(): Unit = with(CachingCatalog(remote, database)) {
        checkedQuery { queryOrganizations(organizationsVisitor, progress) }

        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("organizations"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryOrganizations(any(), eq(progress))
                if (!case.isFailedRemote) {
                    organizations.forEach { sys ->
                        verify(database).addOrganization(sys)
                    }
                    verify(lifetime).update()
                }
            }
            verify(database).queryOrganizations(organizationsVisitor)
        }
    }

    @Test
    fun `test global queryGames`(): Unit = with(CachingCatalog(remote, database)) {
        checkedQuery { queryGames(null, gamesVisitor, progress) }
        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("games@"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryGames(eq(null), any(), eq(progress))
                if (!case.isFailedRemote) {
                    verify(database).addGame(games[0], systems[0], organizations[0])
                    verify(database).addGame(games[1], systems[1], organizations[0])
                    verify(database).addGame(games[2], systems[0], organizations[1])
                    verify(database).addGame(games[3], systems[2], null)
                    verify(lifetime).update()
                }
            }
            verify(database).queryGames(null, gamesVisitor)
        }
    }

    @Test
    fun `test scoped queryGames`(): Unit = with(CachingCatalog(remote, database)) {
        val scope = systems[0].asScope
        checkedQuery { queryGames(scope, gamesVisitor, progress) }
        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("games@sys/0"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryGames(eq(scope), any(), eq(progress))
                if (!case.isFailedRemote) {
                    verify(database).addGame(games[0], systems[0], organizations[0])
                    verify(database).addGame(games[2], systems[0], organizations[1])
                    verify(lifetime).update()
                }
            }
            verify(database).queryGames(scope, gamesVisitor)
        }
    }

    @Test
    fun `test global queryRemixes`(): Unit = with(CachingCatalog(remote, database)) {
        checkedQuery { queryRemixes(null, remixesVisitor, progress) }
        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("remixes@"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryRemixes(eq(null), any(), eq(progress))
                if (!case.isFailedRemote) {
                    verify(database).addRemix(null, remixes[0], games[0])
                    verify(database).addRemix(null, remixes[1], games[0])
                    verify(database).addRemix(null, remixes[2], games[1])
                    verify(database).addRemix(null, remixes[3], games[2])
                    verify(lifetime).update()
                }
            }
            verify(database).queryRemixes(null, remixesVisitor)
        }
    }

    @Test
    fun `test scoped queryRemixes`(): Unit = with(CachingCatalog(remote, database)) {
        val scope = games[0].asScope
        checkedQuery { queryRemixes(scope, remixesVisitor, progress) }
        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("remixes@game/0"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryRemixes(eq(scope), any(), eq(progress))
                if (!case.isFailedRemote) {
                    verify(database).addRemix(scope, remixes[0], games[0])
                    verify(database).addRemix(scope, remixes[1], games[0])
                    verify(lifetime).update()
                }
            }
            verify(database).queryRemixes(scope, remixesVisitor)
        }
    }

    @Test
    fun `test global queryAlbums`(): Unit = with(CachingCatalog(remote, database)) {
        checkedQuery { queryAlbums(null, albumsVisitor, progress) }
        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("albums@"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryAlbums(eq(null), any(), eq(progress))
                if (!case.isFailedRemote) {
                    verify(database).addAlbum(null, albums[0], images[0])
                    verify(database).addAlbum(null, albums[1], images[2])
                    verify(database).addAlbum(null, albums[2], null)
                    verify(database).addAlbum(null, albums[3], images[3])
                    verify(lifetime).update()
                }
            }
            verify(database).queryAlbums(null, albumsVisitor)
        }
    }

    @Test
    fun `test scoped queryAlbums`(): Unit = with(CachingCatalog(remote, database)) {
        val scope = organizations[0].asScope
        checkedQuery { queryAlbums(scope, albumsVisitor, progress) }
        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("albums@org/0"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryAlbums(eq(scope), any(), eq(progress))
                if (!case.isFailedRemote) {
                    verify(database).addAlbum(scope, albums[1], images[2])
                    verify(database).addAlbum(scope, albums[2], null)
                    verify(lifetime).update()
                }
            }
            verify(database).queryAlbums(scope, albumsVisitor)
        }
    }

    @Test
    fun `test findRemixPath succeed`(): Unit = with(CachingCatalog(remote, database)) {
        val id = "remixid"
        checkedQuery { findRemixPath(Remix.Id(id)) }
        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("mp3@remixid"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).findRemixPath(Remix.Id(id))
                if (!case.isFailedRemote) {
                    verify(database).deleteMusicFiles(id)
                    verify(database).addMusicFile(id, musics[0].path, null)
                    verify(lifetime).update()
                }
            }
            verify(database).queryMusicFiles(id)
        }
    }

    @Test
    fun `test findRemixPath failed`(): Unit = with(CachingCatalog(remote, database)) {
        workingRemote.stub {
            on { findRemixPath(any()) } doReturn null
        }
        val id = "remixid"
        checkedQuery { findRemixPath(Remix.Id(id)) }
        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("mp3@remixid"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).findRemixPath(Remix.Id(id))
            }
            verify(database).queryMusicFiles(id)
        }
    }

    @Test
    fun `test queryAlbumTracks succeed`(): Unit = with(CachingCatalog(remote, database)) {
        val id = "albumid"
        checkedQuery { queryAlbumTracks(Album.Id(id), albumTracksVisitor) }
        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("mp3@albumid"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryAlbumTracks(eq(Album.Id(id)), any())
                if (!case.isFailedRemote) {
                    verify(database).deleteMusicFiles(id)
                    musics.forEachIndexed { idx, file ->
                        if (idx > 0) {
                            verify(database).addMusicFile(id, file.path, file.size)
                        }
                    }
                    verify(lifetime).update()
                }
            }
            verify(database).queryMusicFiles(id)
            if (case.hasCache) {
                musics.forEach {
                    verify(albumTracksVisitor).accept(it.path, requireNotNull(it.size))
                }
            }
        }
    }

    @Test
    fun `test queryAlbumTracks failed`(): Unit = with(CachingCatalog(remote, database)) {
        reset(workingRemote)
        val id = "albumid"
        checkedQuery { queryAlbumTracks(Album.Id(id), albumTracksVisitor) }
        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("mp3@albumid"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryAlbumTracks(eq(Album.Id(id)), any())
            }
            verify(database).queryMusicFiles(id)
            if (case.hasCache) {
                musics.forEach {
                    verify(albumTracksVisitor).accept(it.path, requireNotNull(it.size))
                }
            }
        }
    }

    @Test
    fun `test queryGameDetails`(): Unit = with(CachingCatalog(remote, database)) {
        val id = "gameid"
        for (flag in 0..3) {
            val hasChiptune = (flag and 1) != 0
            val hasImage = (flag and 2) != 0
            clearInvocations(*allMocks)
            workingRemote.stub {
                on { queryGameDetails(any()) } doAnswer {
                    Game.Details(
                        if (hasChiptune) musics[1].path else null, if (hasImage) images[0] else null
                    )
                }
            }
            checkedQuery { queryGameDetails(Game.Id(id)) }
            inOrder(*allMocks).run {
                verify(database).getLifetime(eq("gameid"), any())
                verify(lifetime).isExpired
                if (case.isCacheExpired) {
                    verify(database).runInTransaction(any())
                    verify(remote).queryGameDetails(Game.Id(id))
                    if (!case.isFailedRemote) {
                        if (hasImage) {
                            verify(database).addImage(id, images[0])
                        }
                        if (hasChiptune) {
                            verify(database).deleteMusicFiles(id)
                            verify(database).addMusicFile(id, musics[1].path)
                        }
                        if (hasImage || hasChiptune) {
                            verify(lifetime).update()
                        }
                    }
                }
                verify(database).queryImage(id)
                verify(database).queryMusicFiles(id)
            }
            verifyNoMoreInteractions(*allMocks)
        }
    }

    @Test
    fun `test queryAlbumImage succeed`(): Unit = with(CachingCatalog(remote, database)) {
        val id = "albumid"
        checkedQuery { queryAlbumImage(Album.Id(id)) }
        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("albumid"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryAlbumImage(Album.Id(id))
                if (!case.isFailedRemote) {
                    verify(database).addImage(id, images[1])
                    verify(lifetime).update()
                }
            }
            verify(database).queryImage(id)
        }
    }


    @Test
    fun `test queryAlbumImage failed`(): Unit = with(CachingCatalog(remote, database)) {
        workingRemote.stub {
            on { queryAlbumImage(any()) } doReturn null
        }
        val id = "albumid"
        checkedQuery { queryAlbumImage(Album.Id(id)) }
        inOrder(*allMocks).run {
            verify(database).getLifetime(eq("albumid"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryAlbumImage(Album.Id(id))
            }
            verify(database).queryImage(id)
        }
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
            QueriedMusic(FilePath("music/${id}.mp3"), id * 10L)
        }

        private val System.asScope
            get() = Catalog.SystemScope(id)

        private val Organization.asScope
            get() = Catalog.OrganizationScope(id)

        private val Game.asScope
            get() = Catalog.GameScope(id)

        @JvmStatic
        @ParameterizedRobolectricTestRunner.Parameters(name = "{0}")
        fun data() = TestCase.entries.toList()
    }
}
