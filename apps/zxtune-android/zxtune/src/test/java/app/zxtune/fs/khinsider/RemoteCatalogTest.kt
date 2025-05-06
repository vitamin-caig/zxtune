package app.zxtune.fs.khinsider

import androidx.core.net.toUri
import androidx.core.util.Consumer
import app.zxtune.BuildConfig
import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import junit.framework.TestCase.assertEquals
import org.junit.After
import org.junit.Assert.assertNotNull
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.argThat
import org.mockito.kotlin.atLeast
import org.mockito.kotlin.atLeastOnce
import org.mockito.kotlin.eq
import org.mockito.kotlin.geq
import org.mockito.kotlin.inOrder
import org.mockito.kotlin.mock
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class RemoteCatalogTest {
    private lateinit var catalog: RemoteCatalog
    private lateinit var progress: ProgressCallback
    private lateinit var scopesVisitor: Consumer<Scope>
    private lateinit var albumsVisitor: Consumer<AlbumAndDetails>
    private lateinit var albumTracksVisitor: Consumer<TrackAndDetails>

    @Before
    fun setUp() {
        catalog = RemoteCatalog(MultisourceHttpProvider(HttpProviderFactory.createTestProvider()))
        progress = mock {}
        scopesVisitor = mock {}
        albumsVisitor = mock {}
        albumTracksVisitor = mock {}
    }

    @After
    fun tearDown() {
        verifyNoMoreInteractions(
            progress,
            scopesVisitor,
            albumsVisitor,
            albumTracksVisitor,
        )
    }

    @Test
    fun `series list`() {
        val seriesMin = 35
        catalog.queryScopes(Catalog.ScopeType.SERIES, scopesVisitor)
        verify(scopesVisitor).accept(makeScope("animal-crossing", "Animal Crossing"))
        verify(scopesVisitor).accept(makeScope("friday-night-funkin", "Friday Night Funkin'"))
        verify(scopesVisitor).accept(makeScope("zelda", "Zelda"))
        verify(scopesVisitor, atLeast(seriesMin)).accept(any())
    }

    @Test
    fun `platforms list`() {
        val platformsMin = 65
        catalog.queryScopes(Catalog.ScopeType.PLATFORMS, scopesVisitor)
        verify(scopesVisitor).accept(makeScope("game-soundtracks/3do", "3DO"))
        verify(scopesVisitor).accept(
            makeScope(
                "game-soundtracks/sega-mega-drive-genesis", "Genesis / Mega Drive"
            )
        )
        verify(scopesVisitor).accept(makeScope("game-soundtracks/xbox-series-x", "Xbox Series X/S"))
        verify(scopesVisitor, atLeast(platformsMin)).accept(any())
    }

    @Test
    fun `types list`() {
        val typesMin = 7
        catalog.queryScopes(Catalog.ScopeType.TYPES, scopesVisitor)
        verify(scopesVisitor).accept(makeScope("game-soundtracks/gamerips", "Gamerips"))
        verify(scopesVisitor).accept(makeScope("game-soundtracks/arrangements", "Arrangements"))
        verify(scopesVisitor).accept(makeScope("game-soundtracks/inspired-by", "Inspired By"))
        verify(scopesVisitor, atLeast(typesMin)).accept(any())
    }

    @Test
    fun `years list`() {
        val yearsMin = 50
        catalog.queryScopes(Catalog.ScopeType.YEARS, scopesVisitor)
        verify(scopesVisitor).accept(makeScope("game-soundtracks/year/2025", "2025"))
        verify(scopesVisitor).accept(makeScope("game-soundtracks/year/0000", "0000"))
        verify(scopesVisitor, atLeast(yearsMin)).accept(any())
    }

    @Test
    fun `weekly top 40`() {
        val albumsMin = 40
        // No details on top page
        catalog.queryAlbums(Catalog.WEEKLYTOP40, albumsVisitor, progress)
        verify(albumsVisitor).accept(
            makeAlbum(
                "minecraft",
                "Minecraft Soundtrack - Volume Alpha and Beta (Complete Edition) (2011)",
                "",
                "https://vgmsite.com/soundtracks/minecraft/1%20Minecraft%20-%20Volume%20Alpha.jpg"
            )
        )
        verify(albumsVisitor).accept(
            makeAlbum(
                "metal-gear-rising-revengeance-vocal-tracks",
                "METAL GEAR RISING REVENGEANCE Vocal Tracks (2013)",
                "",
                "https://eta.vgmtreasurechest.com/soundtracks/metal-gear-rising-revengeance-vocal-tracks/00%20Front.jpg"
            )
        )
        verify(albumsVisitor, atLeast(albumsMin)).accept(any())
    }

    @Test
    fun `top favorites`() {
        val albums = 98 // WTF?
        catalog.queryAlbums(Catalog.FAVORITES, albumsVisitor, progress)
        verify(albumsVisitor, atLeast(albums)).accept(argThat { image != null })
    }

    @Test
    fun `new 100 albums`() {
        val albums = 100
        catalog.queryAlbums(Catalog.NEW100, albumsVisitor, progress)
        verify(albumsVisitor, atLeast(albums)).accept(any())
    }

    @Test
    fun `platform albums on single page`() {
        val albumsMin = 37
        catalog.queryAlbums(Scope.Id("game-soundtracks/cd-i"), albumsVisitor, progress)
        verify(albumsVisitor).accept(
            makeAlbum(
                "7th-guest-redbook-audio",
                "7th Guest",
                "CD-i, MS-DOS / Gamerip / 1993",
                "https://kappa.vgmsite.com/soundtracks/7th-guest-redbook-audio/7th%20Guest%20CDI%20Sampler.jpg"
            )
        )
        verify(albumsVisitor).accept(
            makeAlbum(
                "zenith-cd-i-gamerip-1996",
                "Zenith",
                "CD-i / Gamerip / 1996",
                "https://kappa.vgmsite.com/soundtracks/zenith-cd-i-gamerip-1996/1%20Zenith-Front.jpg"
            )
        )
        verify(albumsVisitor, atLeast(albumsMin)).accept(any())
    }

    @Test
    fun `albums by letter with special image on two pages`() {
        val albumsMin = 998
        val pagesMin = 3
        catalog.queryAlbums(Catalog.letterScope('U').id, albumsVisitor, progress)
        inOrder(albumsVisitor, progress) {
            verify(albumsVisitor).accept(
                makeAlbum(
                    "u-r-my-nightmare-2020",
                    "U r my Nightmare",
                    "Windows / Arrangement / 2020",
                    "https://vgmsite.com/soundtracks/u-r-my-nightmare-2020/00%20Front.jpg"
                )
            )
            // no image
            verify(albumsVisitor).accept(
                makeAlbum("uboat-soundtrack-2019", "UBOAT Soundtrack", "2019", null)
            )
            // nsfw stub
            verify(albumsVisitor).accept(
                makeAlbum(
                    "ultima-vi-the-false-prophet-ibm-pcxtat-mt32-ibm-pcat-gamerip-1990",
                    "Ultima VI - The False Prophet (IBM PC-XT-AT - MT32)",
                    "IBM PC/AT / Gamerip / 1990",
                    null
                )
            )
            // no type
            verify(albumsVisitor).accept(
                makeAlbum(
                    "under-night-in-birth-exe-late-cl-r-complete-soundtrack",
                    "Under Night In-Birth Exe Late[cl-r] Complete Soundtrack",
                    "PS4 / 2020",
                    "https://kappa.vgmsite.com/soundtracks/under-night-in-birth-exe-late-cl-r-complete-soundtrack/coverart.jpg"
                )
            )
            verify(progress).onProgressUpdate(eq(1), geq(pagesMin))
            // page2
            verify(albumsVisitor).accept(
                makeAlbum(
                    "under-night-in-birth-sys-celes-ps4-ps5-switch-windows-gamerip-2024",
                    "Under Night In-Birth II [Sys:Celes]",
                    "PS4, PS5, Switch, Windows / Gamerip / 2024",
                    "https://eta.vgmtreasurechest.com/soundtracks/under-night-in-birth-sys-celes-ps4-ps5-switch-windows-gamerip-2024/cover.png"
                )
            )/*verify(albumsVisitor).accept(
                makeAlbum("u\u200B\u200Bn-owen-was-phonk-2022", "U.N. OWEN WAS PHONK"),
                FilePath("https://eta.vgmtreasurechest.com/soundtracks/u%E2%80%8B%E2%80%8Bn-owen-was-phonk-2022/folder.jpg")
            )*/
            verify(progress).onProgressUpdate(eq(2), geq(pagesMin))
        }
        verify(albumsVisitor, atLeast(albumsMin)).accept(any())
        verify(progress, atLeast(pagesMin)).onProgressUpdate(any(), geq(pagesMin))
    }

    @Test
    fun `albums by year on several pages`() {
        val albumsMin = 1586
        val pagesMin = 4
        catalog.queryAlbums(Scope.Id("game-soundtracks/year/2024"), albumsVisitor, progress)
        inOrder(albumsVisitor, progress) {
            verify(albumsVisitor).accept(
                makeAlbum(
                    "blud-ps4-switch-windows-xbox-one-gamerip-2024",
                    "#BLUD",
                    "PS4, Switch, Windows, Xbox One / Gamerip / 2024",
                    "https://eta.vgmtreasurechest.com/soundtracks/blud-ps4-switch-windows-xbox-one-gamerip-2024/cover.jpg"
                )
            )
            verify(progress).onProgressUpdate(eq(1), geq(pagesMin))
            verify(albumsVisitor).accept(
                makeAlbum(
                    "friday-night-funkin-hai-yorokonde-2024",
                    "Friday Night Funkin' - Hai Yorokonde",
                    "Online, Windows / Soundtrack / 2024",
                    "https://eta.vgmtreasurechest.com/soundtracks/friday-night-funkin-hai-yorokonde-2024/hai%20yorokonde.png"
                )
            )
            verify(progress).onProgressUpdate(eq(2), geq(pagesMin))
            verify(albumsVisitor).accept(
                makeAlbum(
                    "periculum-windows-gamerip-2024",
                    "Periculum",
                    "Windows / Gamerip / 2024",
                    "https://eta.vgmtreasurechest.com/soundtracks/periculum-windows-gamerip-2024/folder.jpg"
                )
            )
            verify(progress).onProgressUpdate(eq(3), geq(pagesMin))
            verify(albumsVisitor).accept(
                makeAlbum(
                    "warcraft-2-remastered-windows-gamerip-2024",
                    "WarCraft 2 - Remastered",
                    "Windows / Gamerip / 2024",
                    "https://eta.vgmtreasurechest.com/soundtracks/warcraft-2-remastered-windows-gamerip-2024/Cover.jpg"
                )
            )
            verify(progress).onProgressUpdate(eq(4), geq(pagesMin))
        }
        verify(albumsVisitor, atLeast(albumsMin)).accept(any())
        verify(progress, atLeast(pagesMin)).onProgressUpdate(any(), geq(pagesMin))
    }

    @Test
    fun `album tracks without title`() {
        val album = makeAlbum("knight-solitaire-ps-vita-gamerip-2015", "Knight Solitaire")
        assertEquals(
            makeAlbum(
                album,
                "PS Vita / Gamerip / 2015",
                "https://eta.vgmtreasurechest.com/soundtracks/knight-solitaire-ps-vita-gamerip-2015/cover.jpg"
            ), catalog.queryAlbumDetails(
                album.id, albumTracksVisitor
            )
        )
        verify(albumTracksVisitor).accept(
            makeTrack("01.mp3", "Track 1", album, 0, "1:43", "3.05 MB")
        )
        verify(albumTracksVisitor).accept(
            makeTrack("02.mp3", "Track 2", album, 1, "1:51", "3.06 MB")
        )
        verify(albumTracksVisitor).accept(
            makeTrack("03.mp3", "Track 3", album, 2, "4:59", "8.79 MB")
        )
        verify(albumTracksVisitor).accept(
            makeTrack("04.mp3", "Track 4", album, 3, "1:36", "2.80 MB")
        )
    }

    @Test
    fun `aliased album details`() {
        val album = makeAlbum("hebereke-nes", "Hebereke")
        assertEquals(
            makeAlbum(
                album,
                "3DS, NES, PS4, Switch, Wii, Wii U, Windows, Xbox One / Gamerip / 1991",
                "https://kappa.vgmsite.com/soundtracks/hebereke-nes/cover.jpg"
            ), catalog.queryAlbumDetails(
                Album.Id("ufouria-the-saga-nes"), albumTracksVisitor
            )
        )
        verify(albumTracksVisitor).accept(
            makeTrack("01. Title Screen.mp3", "Title Screen", album, 0, "0:55", "1.17 MB")
        )
        verify(albumTracksVisitor, atLeast(14)).accept(any())
    }

    @Test
    fun `album tracks with title`() {
        val album = makeAlbum(
            "terminator-2-judgement-day-master-system-gamerip-1993", "Terminator 2 - Judgement Day"
        )
        assertEquals(
            makeAlbum(
                album,
                "Master System / Gamerip / 1993",
                "https://kappa.vgmsite.com/soundtracks/terminator-2-judgement-day-master-system-gamerip-1993/Terminator%202%20-%20Judgement%20Day%20Title%20Screen.jpg"
            ), catalog.queryAlbumDetails(
                album.id, albumTracksVisitor
            )
        )
        verify(albumTracksVisitor).accept(
            makeTrack(
                "01. Title, Story.mp3", "Title, Story", album, 0, "1:10", "0.73 MB"
            )
        )
        verify(albumTracksVisitor).accept(
            makeTrack(
                "02. Truck Area.mp3", "Truck Area", album, 1, "0:32", "0.42 MB"
            )
        )
        verify(albumTracksVisitor).accept(
            makeTrack(
                "03. The Corral, Pescadero State Hospital, Cyberdyne HQ.mp3",
                "The Corral, Pescadero State Hospital, Cyberdyne HQ",
                album,
                2,
                "0:47",
                "0.58 MB"
            )
        )
        verify(albumTracksVisitor).accept(
            makeTrack(
                "04. Steel Mill.mp3", "Steel Mill", album, 3, "0:57", "0.71 MB"
            )
        )
    }

    @Test
    fun `album tracks without index`() {
        val album = makeAlbum(
            "contra-returns-android-ios-mobile-gamerip-2021", "Contra Returns"
        )
        assertEquals(
            makeAlbum(
                album,
                "Android, iOS, Mobile / Gamerip / 2021",
                "https://eta.vgmtreasurechest.com/soundtracks/contra-returns-android-ios-mobile-gamerip-2021/cover.jpg"
            ), catalog.queryAlbumDetails(
                album.id, albumTracksVisitor
            )
        )
        verify(albumTracksVisitor).accept(
            makeTrack(
                "BGM BloodFalcon Boss 1.mp3", "BGM BloodFalcon Boss 1", album, 0, "0:56", "1.44 MB"
            )
        )
        verify(albumTracksVisitor).accept(
            makeTrack(
                "XiaoGuai kill success general.mp3",
                "XiaoGuai kill success general",
                album,
                169,
                "0:08",
                "0.20 MB"
            )
        )
        verify(albumTracksVisitor, atLeast(170)).accept(any())
    }

    @Test
    fun `track location`() {
        assertTrue(
            catalog.findTrackLocation(
                Album.Id("terminator-2-judgement-day-master-system-gamerip-1993"),
                Track.Id("02. Truck Area.mp3")
            ).toString().matches(
                "^https://[^/]+/soundtracks/terminator-2-judgement-day-master-system-gamerip-1993/[^/]+/02.%20Truck%20Area.mp3".toRegex()
            )
        )
    }

    @Test
    fun `random album`() {
        assertNotNull(catalog.queryAlbumDetails(Catalog.RANDOM_ALBUM, albumTracksVisitor))
        verify(albumTracksVisitor, atLeastOnce()).accept(any())
    }

    @Test
    fun `remote uris`() {
        val image =
            FilePath("https://eta.vgmtreasurechest.com/soundtracks/000-2008/00%20Front.jpg".toUri())
        RemoteCatalog.getRemoteUris(image).run {
            assertEquals(2, size)
            assertEquals(
                "${BuildConfig.CDN_ROOT}/download/khinsider/soundtracks/000-2008/00%20Front.jpg",
                get(0).toString()
            )
            assertEquals(image.value, get(1))
        }
        assertEquals(
            "https://eta.vgmtreasurechest.com/soundtracks/000-2008/thumbs/00%20Front.jpg".toUri(),
            image.thumbUri
        )
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/khinsider/soundtracks/09%EF%BC%8F09%EF%BC%8F19-2019/(01)%20%5BMoonie%5D%2009-09-19.mp3",
            RemoteCatalog.getRemoteUri(
                Album.Id("09／09／19-2019"), Track.Id("(01) [Moonie] 09-09-19.mp3")
            ).toString()
        )
    }
}

private fun makeScope(id: String, title: String) = Scope(Scope.Id(id), title)
private fun makeAlbum(id: String, title: String) = Album(Album.Id(id), title)
private fun makeAlbum(id: String, title: String, details: String, image: String?) =
    makeAlbum(makeAlbum(id, title), details, image)

private fun makeAlbum(album: Album, details: String, image: String?) =
    AlbumAndDetails(album, details, image?.let { FilePath(it.toUri()) })

private fun makeTrack(id: String, title: String) = Track(Track.Id(id), title)
private fun makeTrack(
    id: String, title: String, album: Album, idx: Int, duration: String, size: String
) = TrackAndDetails(album, makeTrack(id, title), idx, duration, size)
