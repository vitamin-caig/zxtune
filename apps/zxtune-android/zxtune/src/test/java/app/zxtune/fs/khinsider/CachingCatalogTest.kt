package app.zxtune.fs.khinsider

import androidx.core.net.toUri
import androidx.core.util.Consumer
import app.zxtune.fs.CachingCatalogTestBase
import app.zxtune.fs.dbhelpers.Utils
import app.zxtune.utils.ProgressCallback
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.eq
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.verifyNoMoreInteractions
import org.mockito.kotlin.whenever
import org.robolectric.ParameterizedRobolectricTestRunner

@RunWith(ParameterizedRobolectricTestRunner::class)
class CachingCatalogTest(case: TestCase) : CachingCatalogTestBase(case) {
    private val scopes = Array(4, ::makeScope)
    private val albums = Array(10, ::makeAlbumAndDetails)
    private val details = albums[1]
    private val tracks = Array(5) { id ->
        makeTrackDetails(details.album, id)
    }

    private val database = mock<Database> {
        on { runInTransaction(any()) } doAnswer {
            it.getArgument<Utils.ThrowingRunnable>(0).run()
        }
        on { getLifetime(any(), any()) } doReturn lifetime
        on { queryScopes(any(), any()) } doReturn case.hasCache
        on { queryAlbums(any(), any()) } doReturn case.hasCache
        on { queryAlbum(any()) } doReturn if (case.hasCache) details else null
        on { queryRandomAlbum() } doReturn if (case.hasCache) details else null
        on { queryTracks(any(), any()) } doReturn case.hasCache
    }
    private val workingRemote = mock<RemoteCatalog> {
        on { queryScopes(any(), any()) } doAnswer {
            with(it.getArgument<Consumer<Scope>>(1)) {
                scopes.forEach(this::accept)
            }
        }
        on { queryAlbums(any(), any(), any()) } doAnswer {
            val visitor = it.getArgument<Consumer<AlbumAndDetails>>(1)
            val progress = it.getArgument<ProgressCallback>(2)
            val total = albums.size
            albums.forEachIndexed { idx, album ->
                // 1 per page
                visitor.accept(album)
                progress.onProgressUpdate(idx + 1, total)
            }
        }
        on { queryAlbumDetails(any(), any()) } doAnswer {
            with(it.getArgument<Consumer<TrackAndDetails>>(1)) {
                tracks.forEach(this::accept)
            }
            details
        }
    }
    private val failedRemote = mock<RemoteCatalog>(defaultAnswer = { throw error })
    private val remote = if (case.isFailedRemote) failedRemote else workingRemote
    private val scopesVisitor = mock<Consumer<Scope>>()
    private val albumsVisitor = mock<Consumer<AlbumAndDetails>>()
    private val tracksVisitor = mock<Consumer<TrackAndDetails>>()

    private val isNetworkAvailable = case.isCacheExpired

    init {
        doAnswer { isNetworkAvailable }.whenever(remote).isNetworkAvailable()
    }

    private val allMocks = arrayOf(
        lifetime,
        database,
        failedRemote,
        workingRemote,
        progress,
        scopesVisitor,
        albumsVisitor,
        tracksVisitor,
    )

    @After
    fun tearDown() = verifyNoMoreInteractions(*allMocks)

    @Test
    fun `test queryScopes`() = with(CachingCatalog(remote, database)) {
        val scopeType = Catalog.ScopeType.SERIES
        checkedQuery { queryScopes(scopeType, scopesVisitor) }

        inOrder(*allMocks) {
            verify(database).getLifetime(eq("series"), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(remote).queryScopes(eq(scopeType), any())
                if (!case.isFailedRemote) {
                    scopes.forEach {
                        verify(database).addScope(scopeType, it)
                    }
                    verify(lifetime).update()
                }
            }
            verify(database).queryScopes(scopeType, scopesVisitor)
        }
    }

    @Test
    fun `test queryAlbums`() = with(CachingCatalog(remote, database)) {
        val queriedScope = scopes[1]
        checkedQuery { queryAlbums(queriedScope.id, albumsVisitor, progress) }

        inOrder(*allMocks) {
            verify(database).getLifetime(eq(queriedScope.id.value), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(database).cleanupScope(queriedScope.id)
                verify(remote).queryAlbums(eq(queriedScope.id), any(), eq(progress))
                if (!case.isFailedRemote) {
                    albums.forEachIndexed { idx, album ->
                        verify(database).addAlbum(queriedScope.id, album)
                        verify(progress).onProgressUpdate(idx + 1, 10)
                    }
                    verify(lifetime).update()
                }
            }
            verify(database).queryAlbums(queriedScope.id, albumsVisitor)
        }
    }

    @Test
    fun `test queryAlbumDetails`() = with(CachingCatalog(remote, database)) {
        checkedQuery {
            val result = if (case.hasCache) details else null
            assertEquals(result, queryAlbumDetails(details.album.id, tracksVisitor))
        }

        inOrder(*allMocks) {
            verify(database).getLifetime(eq(details.album.id.value), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(database).cleanupAlbum(details.album.id)
                verify(remote).queryAlbumDetails(eq(details.album.id), any())
                if (!case.isFailedRemote) {
                    tracks.forEach {
                        verify(database).addTrack(it)
                    }
                    verify(database).addAlbum(details)
                    verify(lifetime).update()
                }
            }
            verify(database).queryAlbum(details.album.id)
            if (case.hasCache) {
                verify(database).queryTracks(details.album, tracksVisitor)
            }
        }
    }

    @Test
    fun `test queryAlbumDetails for random album`() = with(CachingCatalog(remote, database)) {
        checkedQuery {
            val hasResult = if (isNetworkAvailable) !case.isFailedRemote else case.hasCache
            val result = if (hasResult) details else null
            assertEquals(result, queryAlbumDetails(Catalog.RANDOM_ALBUM, tracksVisitor))
        }

        inOrder(*allMocks) {
            verify(remote).isNetworkAvailable()
            if (isNetworkAvailable) {
                verify(database).runInTransaction(any())
                verify(remote).queryAlbumDetails(eq(Catalog.RANDOM_ALBUM), any())
                if (!case.isFailedRemote) {
                    verify(database).cleanupAlbum(details.album.id)
                    tracks.forEach {
                        verify(database).addTrack(it)
                        verify(tracksVisitor).accept(it)
                    }
                    verify(database).addAlbum(details)
                    verify(database).getLifetime(eq(details.album.id.value), any())
                    verify(lifetime).update()
                }
            } else {
                verify(database).queryRandomAlbum()
                if (case.hasCache) {
                    verify(database).queryTracks(details.album, tracksVisitor)
                }
            }
        }
    }

    @Test
    fun `test queryAlbumDetails for aliased album`() = with(CachingCatalog(remote, database)) {
        val aliasedAlbum = albums[2].album
        checkedQuery {
            val result = if (case.hasCache) details else null
            assertEquals(result, queryAlbumDetails(aliasedAlbum.id, tracksVisitor))
        }

        inOrder(*allMocks) {
            verify(database).getLifetime(eq(aliasedAlbum.id.value), any())
            verify(lifetime).isExpired
            if (case.isCacheExpired) {
                verify(database).runInTransaction(any())
                verify(database).cleanupAlbum(aliasedAlbum.id)
                verify(remote).queryAlbumDetails(eq(aliasedAlbum.id), any())
                if (!case.isFailedRemote) {
                    verify(database).cleanupAlbum(details.album.id)
                    verify(database).addAlbumAlias(aliasedAlbum.id, details.album.id)
                    tracks.forEach {
                        verify(database).addTrack(it)
                    }
                    verify(database).addAlbum(details)
                    verify(lifetime).update()
                    verify(database).getLifetime(eq(details.album.id.value), any())
                    verify(lifetime).update()
                }
            }
            verify(database).queryAlbum(aliasedAlbum.id)
            if (case.hasCache) {
                verify(database).queryTracks(details.album, tracksVisitor)
            }
        }
    }

    companion object {
        private fun makeScope(id: Int) = Scope(Scope.Id("scope/$id"), "Scope $id")
        private fun makeAlbumAndDetails(id: Int) = AlbumAndDetails(
            makeAlbum(id),
            "Details for album $id",
            if (id and 1 == 0) FilePath("https://host/path/to/image_$id.png".toUri()) else null
        )

        private fun makeAlbum(id: Int) = Album(Album.Id("album/$id"), "Album $id")
        private fun makeTrack(id: Int) = Track(Track.Id("track_$id.mp3"), "Track $id")
        private fun makeTrackDetails(album: Album, id: Int) =
            TrackAndDetails(album, makeTrack(id), id, "$id:00", "$id KB")

        @JvmStatic
        @ParameterizedRobolectricTestRunner.Parameters(name = "{0}")
        fun data() = TestCase.entries
    }
}
