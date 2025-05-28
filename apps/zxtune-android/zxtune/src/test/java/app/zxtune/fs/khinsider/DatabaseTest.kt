package app.zxtune.fs.khinsider

import androidx.core.net.toUri
import androidx.core.util.Consumer
import androidx.room.Room
import androidx.test.core.app.ApplicationProvider
import app.zxtune.assertThrows
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertSame
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.mock
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoInteractions
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
    fun tearDown() = underTest.close()

    @Test
    fun `test runInTransaction`() {
        val error = IOException("Error")
        assertSame(error, assertThrows {
            underTest.runInTransaction {
                `test empty database`()
                throw error
            }
        })
        `test empty database`()
    }

    @Test
    fun `test empty database`() = with(underTest) {
        val scopes = mock<Consumer<Scope>>().apply {
            assertFalse(queryScopes(Catalog.ScopeType.PLATFORMS, this))
        }
        val albums = mock<Consumer<AlbumAndDetails>>().apply {
            assertFalse(queryAlbums(Scope.Id("somescope"), this))
        }
        val tracks = mock<Consumer<TrackAndDetails>>().apply {
            assertFalse(queryTracks(makeAlbum(0), this))
        }
        assertEquals(null, queryRandomAlbum())
        assertEquals(null, queryAlbum(Album.Id("somealbum")))
        verifyNoInteractions(scopes, albums, tracks)
    }

    @Test
    fun `test scopes`() = with(underTest) {
        Catalog.ScopeType.entries.forEach { type ->
            addScope(type, makeScope(type.name))
        }
        Catalog.ScopeType.entries.forEach { type ->
            mock<Consumer<Scope>>().apply {
                assertTrue(queryScopes(type, this))
            }.also {
                verify(it).accept(makeScope(type.name))
                verifyNoMoreInteractions(it)
            }
        }
    }

    @Test
    fun `test scoped albums`() = with(underTest) {
        val scope1 = makeScope("scope1")
        (0..5).forEach { id ->
            addAlbum(scope1.id, makeAlbumAndDetails(id))
        }
        // overlapped
        val scope2 = makeScope("scope2")
        (3..8).forEach { id ->
            addAlbum(scope2.id, makeAlbumAndDetails(id))
        }
        withMock<Consumer<AlbumAndDetails>> {
            assertTrue(queryAlbums(scope1.id, this))
            (0..5).forEach { id ->
                verify(this).accept(makeAlbumAndDetails(id))
            }
        }
        withMock<Consumer<AlbumAndDetails>> {
            assertTrue(queryAlbums(scope2.id, this))
            (3..8).forEach { id ->
                verify(this).accept(makeAlbumAndDetails(id))
            }
        }
        cleanupScope(scope1.id)
        withMock<Consumer<AlbumAndDetails>> {
            assertFalse(queryAlbums(scope1.id, this))
        }
        withMock<Consumer<AlbumAndDetails>> {
            assertTrue(queryAlbums(scope2.id, this))
            (3..8).forEach { id ->
                verify(this).accept(makeAlbumAndDetails(id))
            }
        }
    }

    @Test
    fun `test albums`() = with(underTest) {
        val scope = makeScope("scope1")
        val toReplace = AlbumAndDetails(makeAlbum(0), "Overwritten", null).also {
            addAlbum(scope.id, it)
            assertEquals(it, queryRandomAlbum())
        }
        val toRealias = AlbumAndDetails(makeAlbum(6), "Overwritten", null).also {
            addAlbum(scope.id, it)
            queryRandomAlbum().let { random ->
                assert(random == it || random == toReplace)
            }
        }
        withMock<Consumer<AlbumAndDetails>> {
            assertTrue(queryAlbums(scope.id, this))
            verify(this).accept(toReplace)
            verify(this).accept(toRealias)
        }
        val albums = Array(5, ::makeAlbumAndDetails).also {
            it.forEach(this::addAlbum)
        }
        val aliases = Array(5) { idx ->
            Album.Id("album/${idx + 5}").also {
                addAlbumAlias(it, albums[idx].album.id)
            }
        }
        albums.forEach {
            assertEquals(it, queryAlbum(it.album.id))
        }
        aliases.forEachIndexed { idx, alias ->
            assertEquals(albums[idx], queryAlbum(alias))
        }
        withMock<Consumer<AlbumAndDetails>> {
            assertTrue(queryAlbums(scope.id, this))
            // overwritten -> album[0]
            verify(this).accept(albums[0])
            // overaliased -> aliases[1] -> album[1]
            verify(this).accept(albums[1])
        }
        // should not overwrite
        addAlbum(scope.id, toReplace)
        addAlbum(scope.id, toRealias)
        albums.forEach {
            assertEquals(it, queryAlbum(it.album.id))
        }
        aliases.forEachIndexed { idx, alias ->
            assertEquals(albums[idx], queryAlbum(alias))
        }
        withMock<Consumer<AlbumAndDetails>> {
            assertTrue(queryAlbums(scope.id, this))
            verify(this).accept(albums[0])
            verify(this).accept(albums[1])
        }
        // and this should
        addAlbum(toReplace)
        addAlbum(toRealias)
        albums.forEachIndexed { idx, album ->
            assertEquals(if (idx == 0) toReplace else album, queryAlbum(album.album.id))
        }
        aliases.forEachIndexed { idx, alias ->
            val expected = when (idx) {
                0 -> toReplace
                1 -> toRealias
                else -> albums[idx]
            }
            assertEquals(expected, queryAlbum(alias))
        }
        withMock<Consumer<AlbumAndDetails>> {
            assertTrue(queryAlbums(scope.id, this))
            verify(this).accept(toReplace)
            verify(this).accept(toRealias)
        }
    }

    @Test
    fun `test tracks`() = with(underTest) {
        val album1 = makeAlbum(1)
        val album2 = makeAlbum(2)
        val tracks = Array(12) {
            makeTrackDetails(if (it < 5) album1 else album2, it)
        }.also {
            it.forEach(this::addTrack)
        }
        withMock<Consumer<TrackAndDetails>> {
            assertTrue(queryTracks(album1, this))
            (0..4).forEach {
                verify(this).accept(tracks[it])
            }
        }
        withMock<Consumer<TrackAndDetails>> {
            assertTrue(queryTracks(album2, this))
            (5..11).forEach {
                verify(this).accept(tracks[it])
            }
        }
        cleanupAlbum(album1.id)
        withMock<Consumer<TrackAndDetails>> {
            assertFalse(queryTracks(album1, this))
        }
        withMock<Consumer<TrackAndDetails>> {
            assertTrue(queryTracks(album2, this))
            (5..11).forEach {
                verify(this).accept(tracks[it])
            }
        }
    }

    companion object {
        private fun makeScope(id: String) = Scope(Scope.Id("scope/$id"), "Scope $id")
        private fun makeAlbumAndDetails(id: Int) = AlbumAndDetails(
            makeAlbum(id),
            "Details for album $id",
            if (id and 1 == 0) FilePath("https://host/path/to/image_$id.png".toUri()) else null
        )

        private fun makeAlbum(id: Int) = Album(Album.Id("album/$id"), "Album $id")
        private fun makeTrack(id: Int) = Track(Track.Id("track_$id.mp3"), "Track $id")
        private fun makeTrackDetails(album: Album, id: Int) =
            TrackAndDetails(album, makeTrack(id), id, "$id:00", "$id KB")

        private inline fun <reified T : Any> withMock(block: T.() -> Unit) = mock<T>().run {
            block()
            verifyNoMoreInteractions(this)
        }
    }
}
