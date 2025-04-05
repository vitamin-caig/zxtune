package app.zxtune.fs.ocremix

import app.zxtune.BuildConfig
import app.zxtune.assertThrows
import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.AdditionalMatchers.geq
import org.mockito.kotlin.any
import org.mockito.kotlin.anyOrNull
import org.mockito.kotlin.atLeast
import org.mockito.kotlin.eq
import org.mockito.kotlin.mock
import org.mockito.kotlin.times
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner
import java.io.FileNotFoundException

@RunWith(RobolectricTestRunner::class)
class RemoteCatalogTest {
    private lateinit var catalog: RemoteCatalog
    private lateinit var progress: ProgressCallback
    private lateinit var systemsVisitor: Catalog.Visitor<System>
    private lateinit var organizationsVisitor: Catalog.Visitor<Organization>
    private lateinit var gamesVisitor: Catalog.GamesVisitor
    private lateinit var remixesVisitor: Catalog.RemixesVisitor
    private lateinit var albumsVisitor: Catalog.AlbumsVisitor
    private lateinit var albumTracksVisitor: Catalog.AlbumTracksVisitor

    @Before
    fun setUp() {
        catalog = RemoteCatalog(MultisourceHttpProvider(HttpProviderFactory.createTestProvider()))
        progress = mock {}
        systemsVisitor = mock {}
        organizationsVisitor = mock {}
        gamesVisitor = mock {}
        remixesVisitor = mock {}
        albumsVisitor = mock {}
        albumTracksVisitor = mock {}
    }

    @After
    fun tearDown() {
        verifyNoMoreInteractions(
            progress,
            systemsVisitor,
            organizationsVisitor,
            gamesVisitor,
            remixesVisitor,
            albumsVisitor,
            albumTracksVisitor,
        )
    }

    @Test
    fun `test systems`() {
        val systemsMin = 79
        catalog.querySystems(systemsVisitor)
        verify(systemsVisitor).accept(sys("3do", "3DO"))
        verify(systemsVisitor).accept(sys("axlxe", "Atari 400/800/XL/XE"))
        verify(systemsVisitor).accept(sys("spec", "ZX Spectrum"))
        verify(systemsVisitor, atLeast(systemsMin)).accept(any())
    }

    @Test
    fun `test organizations`() {
        val orgsMin = 300
        catalog.queryOrganizations(organizationsVisitor, progress)
        verify(organizationsVisitor).accept(org("2/nintendo", "Nintendo"))
        verify(progress).onProgressUpdate(250, orgsMin)
        verify(organizationsVisitor).accept(org("3502/zeboyd-games", "Zeboyd Games"))
        verify(organizationsVisitor, atLeast(257)).accept(any())
        verify(progress, atLeast(5)).onProgressUpdate(any(), geq(orgsMin))
    }

    @Test
    fun `test all games`() {
        val gamesMin = 1354
        catalog.queryGames(null, gamesVisitor, progress)

        //page1
        verify(gamesVisitor).accept(
            game("43663/007-tomorrow-never-dies-ps1", "007: Tomorrow Never Dies"),
            sys("ps1", "PlayStation"),
            org("21/electronic-arts", "Electronic Arts"),
            FilePath("images/games/ps1/3/007-tomorrow-never-dies-ps1-title-81143.png"),
        )
        verify(gamesVisitor).accept(
            game("450/artura-c64", "Artura"),
            sys("c64", "Commodore 64"),
            org("26/gremlin", "Gremlin"),
            FilePath("images/games/c64/0/artura-c64-title-77275.gif"),
        )
        //page2
        verify(progress).onProgressUpdate(eq(50), geq(gamesMin))
        verify(gamesVisitor).accept(
            game("95511/assassins-creed-brotherhood-ps3", "Assassin's Creed Brotherhood"),
            sys("ps3", "PlayStation 3"),
            org("68/ubisoft", "Ubisoft"),
            FilePath("images/games/ps3/1/assassins-creed-brotherhood-ps3-title-80024.png"),
        )
        verify(gamesVisitor).accept(
            game("103/black-and-white-win", "Black & White"),
            sys("win", "Windows"),
            org("21/electronic-arts", "Electronic Arts"),
            FilePath("images/games/win/3/black-and-white-win-title-81141.png"),
        )
        //page3
        verify(progress).onProgressUpdate(eq(100), geq(gamesMin))
        verify(gamesVisitor).accept(
            game("87/blast-corps-n64", "Blast Corps"),
            sys("n64", "Nintendo 64"),
            org("2/nintendo", "Nintendo"),
            FilePath("images/games/n64/7/blast-corps-n64-title-613.gif"),
        )
        verify(gamesVisitor).accept(
            game("104/castlevania-n64", "Castlevania"),
            sys("n64", "Nintendo 64"),
            org("5/konami", "Konami"),
            FilePath("images/games/n64/4/castlevania-n64-title-31440.gif"),
        )
        //page4
        verify(progress).onProgressUpdate(eq(150), geq(gamesMin))
        verify(gamesVisitor).accept(
            game("18/castlevania-nes", "Castlevania"),
            sys("nes", "NES"),
            org("5/konami", "Konami"),
            FilePath("images/games/nes/8/castlevania-nes-title-80953.png"),
        )

        //last
        verify(gamesVisitor).accept(
            game(
                "8437/zool-ninja-of-the-nth-dimension-amiga", "Zool: Ninja of the \"Nth\" Dimension"
            ),
            sys("amiga", "Amiga"),
            org("26/gremlin", "Gremlin"),
            FilePath("images/games/amiga/7/zool-ninja-of-the-nth-dimension-amiga-title-78080.jpg"),
        )

        verify(gamesVisitor, atLeast(gamesMin)).accept(any(), any(), anyOrNull(), anyOrNull())
        verify(progress, atLeast(gamesMin / 50)).onProgressUpdate(any(), geq(gamesMin))
    }

    @Test
    fun `test system games`() {
        val gamesMin = 164
        val sys = sys("win", "Windows")
        catalog.queryGames(Catalog.SystemScope(sys.id), gamesVisitor, progress)
        //page1
        verify(gamesVisitor).accept(
            game("943/3-d-ultra-pinball-creep-night-win", "3-D Ultra Pinball: Creep Night"),
            sys,
            org("58/sierra", "Sierra"),
            FilePath("images/games/win/3/3-d-ultra-pinball-creep-night-win-title-77991.jpg"),
        )
        verify(gamesVisitor).accept(
            game("714/binding-of-isaac-win", "The Binding of Isaac"),
            sys,
            null, // no organization!
            FilePath("images/games/win/4/binding-of-isaac-win-title-71480.jpg"),
        )
        //page2
        verify(progress).onProgressUpdate(eq(50), geq(gamesMin))
        verify(gamesVisitor).accept(
            game("553/elder-scrolls-iv-oblivion-win", "The Elder Scrolls IV: Oblivion"),
            sys,
            org("137/2k-games", "2K Games"),
            FilePath("images/games/win/3/elder-scrolls-iv-oblivion-win-title-77037.jpg"),
        )
        verify(gamesVisitor).accept(
            game("539/myst-iii-exile-win", "Myst III: Exile"),
            sys,
            org("68/ubisoft", "Ubisoft"),
            FilePath("images/games/win/9/myst-iii-exile-win-title-79405.png"),
        )
        verify(gamesVisitor, atLeast(gamesMin)).accept(any(), any(), anyOrNull(), anyOrNull())
        verify(progress, atLeast(gamesMin / 50)).onProgressUpdate(any(), geq(gamesMin))
    }

    @Test
    fun `test organization games`() {
        val gamesMin = 9
        val org = org("43/midway", "Midway")
        catalog.queryGames(Catalog.OrganizationScope(org.id), gamesVisitor, progress)
        verify(gamesVisitor).accept(
            game("2956/doom-64-n64", "Doom 64"),
            sys("n64", "Nintendo 64"),
            org,
            FilePath("images/games/n64/6/doom-64-n64-title-81043.png"),
        )
        verify(gamesVisitor).accept(
            game("966/shadow-hearts-ps2", "Shadow Hearts"),
            sys("ps2", "PlayStation 2"),
            org,
            FilePath("images/games/ps2/6/shadow-hearts-ps2-title-80179.png"),
        )
        verify(gamesVisitor, atLeast(gamesMin)).accept(any(), any(), anyOrNull(), anyOrNull())
    }

    @Test
    fun `test all remixes`() {
        val remixesMin = 4570
        catalog.queryRemixes(null, remixesVisitor, progress)
        verify(remixesVisitor).accept(
            remix("OCR04768", "Frozen Light"), game("6/final-fantasy-vi-snes", "Final Fantasy VI")
        )
        verify(progress).onProgressUpdate(eq(30), geq(remixesMin))
        verify(remixesVisitor).accept(
            remix("OCR00006", "Legacy"), game(
                "95/phantasy-star-iii-generations-of-doom-gen",
                "Phantasy Star III: Generations of Doom"
            )
        )

        verify(remixesVisitor, atLeast(remixesMin)).accept(any(), any())
        verify(progress, atLeast(remixesMin / 30)).onProgressUpdate(any(), geq(remixesMin))
    }

    @Test
    fun `test game remixes`() {
        val remixesMin = 9
        val mainGame = game("50/ducktales-nes", "DuckTales")
        catalog.queryRemixes(Catalog.GameScope(mainGame.id), remixesVisitor, progress)

        verify(remixesVisitor).accept(
            remix("OCR03284", "Apollo Duck"), mainGame
        )
        verify(remixesVisitor).accept(
            remix("OCR03929", "To the Moon or Bust!"), mainGame
        )
        verify(remixesVisitor).accept(
            remix("OCR04485", "wily theme"), game("2/mega-man-2-nes", "Mega Man 2")
        )
        verify(remixesVisitor, atLeast(remixesMin)).accept(any(), any())
    }

    @Test
    fun `test organization remixes`() {
        val remixesMin = 45
        catalog.queryRemixes(
            Catalog.OrganizationScope(Organization.Id("22/enix")), remixesVisitor, progress
        )

        verify(remixesVisitor).accept(
            remix("OCR00541", "Fill More Funk"), game("25/actraiser-snes", "ActRaiser")
        )
        verify(remixesVisitor).accept(
            remix("OCR03456", "Home"), game("257/terranigma-snes", "Terranigma")
        )
        verify(progress).onProgressUpdate(eq(30), geq(remixesMin))
        // same game but next page
        verify(remixesVisitor).accept(
            remix("OCR04037", "Skaði, Fjallið Veiði Gyðja"),
            game("257/terranigma-snes", "Terranigma")
        )
        verify(remixesVisitor).accept(
            remix("OCR01790", "Yggdrasil Speaks to Me"),
            game("547/valkyrie-profile-ps1", "Valkyrie Profile")
        )
        verify(remixesVisitor, atLeast(remixesMin)).accept(any(), any())
    }

    @Test
    fun `test system remixes`() {
        val remixesMin = 33
        catalog.queryRemixes(Catalog.SystemScope(System.Id("amiga")), remixesVisitor, progress)

        verify(remixesVisitor).accept(
            remix("OCR01512", "AmberTrance"), game("484/amberstar-amiga", "Amberstar")
        )
        verify(progress).onProgressUpdate(eq(30), geq(remixesMin))
        verify(remixesVisitor).accept(
            remix("OCR03136", "Ninja-Godteri"), game(
                "8437/zool-ninja-of-the-nth-dimension-amiga", "Zool: Ninja of the \"Nth\" Dimension"
            )
        )
        verify(remixesVisitor, atLeast(remixesMin)).accept(any(), any())
    }

    @Test
    fun `test all albums`() {
        val albumsMin = 148
        catalog.queryAlbums(null, albumsVisitor, progress)

        verify(albumsVisitor).accept(
            album("60065/the-impact-of-iwata", "The Impact of Iwata"),
            FilePath("images/albums/5/8/60065-258.png"),
        )
        verify(progress).onProgressUpdate(eq(30), geq(albumsMin))
        verify(albumsVisitor).accept(
            album(
                "71/heart-of-a-gamer-a-tribute-to-satoru-iwata",
                "Heart of a Gamer: A Tribute to Satoru Iwata"
            ),
            FilePath("images/albums/1/5/71-185.png"),
        )
        //last
        verify(albumsVisitor).accept(
            album("5306/instant-remedy", "Instant Remedy"),
            FilePath("images/albums/6/4/5306-74.jpg"),
        )
        verify(albumsVisitor, atLeast(albumsMin)).accept(any(), any())
        verify(progress, atLeast(albumsMin / 30)).onProgressUpdate(any(), geq(albumsMin))
    }

    @Test
    fun `test game albums`() {
        val albumsMin = 9
        catalog.queryAlbums(
            Catalog.GameScope(Game.Id("19/castlevania-ii-simons-quest-nes")),
            albumsVisitor,
            progress
        )

        verify(albumsVisitor).accept(
            album("86/sophomore-year", "Sophomore Year"),
            FilePath("images/albums/6/2/86-222.png"),
        )
        verify(albumsVisitor).accept(
            album("4347/select-start", "Select Start"), FilePath("images/albums/7/6/4347-66.jpg")
        )
        verify(albumsVisitor, atLeast(albumsMin)).accept(any(), any())
    }

    @Test
    fun `test organization albums`() {
        val albumsMin = 65
        catalog.queryAlbums(
            Catalog.OrganizationScope(Organization.Id("2/nintendo")), albumsVisitor, progress
        )

        verify(albumsVisitor).accept(
            album("60065/the-impact-of-iwata", "The Impact of Iwata"),
            FilePath("images/albums/5/8/60065-258.png"),
        )
        verify(progress).onProgressUpdate(eq(50), geq(albumsMin))
        verify(albumsVisitor).accept(
            album("4700/nesperado", "NESperado"),
            FilePath("images/albums/0/0/4700-70.jpg"),
        )
        verify(albumsVisitor, atLeast(albumsMin)).accept(any(), any())
    }

    @Test
    fun `test system albums`() {
        val albumsMin = 6
        catalog.queryAlbums(
            Catalog.SystemScope(System.Id("sat")), albumsVisitor, progress
        )

        verify(albumsVisitor).accept(
            album("86/sophomore-year", "Sophomore Year"),
            FilePath("images/albums/6/2/86-222.png"),
        )
        verify(albumsVisitor).accept(
            album("4081/earthworm-jim-anthology", "Earthworm Jim Anthology"),
            FilePath("images/albums/1/4/4081-64.jpg"),
        )
        verify(albumsVisitor, atLeast(albumsMin)).accept(any(), any())
    }

    @Test
    fun `test remix path`() {
        assertEquals(
            FilePath("music/remixes/Final_Fantasy_6_Frozen_Light_OC_ReMix.mp3"),
            catalog.findRemixPath(Remix.Id("OCR04768"))
        )
    }

    @Test
    fun `test empty album`() {
        catalog.queryAlbumTracks(Album.Id("5710/leg-vacuum"), albumTracksVisitor)
    }

    @Test
    fun `test album tracks`() {
        catalog.queryAlbumTracks(
            Album.Id("30/nights-lucid-dreaming"), albumTracksVisitor
        )
        verify(albumTracksVisitor).accept(
            FilePath(
                "albums/NiGHTS - Lucid Dreaming/MP3/Disc 1 - 1st Dream/1-12 Tomorrow Should Have Been Last Night (After The Dream) [OA, DragonAvenger].mp3"
            ), 6056168L
        )
        verify(albumTracksVisitor, times(27)).accept(any<FilePath>(), geq(1000000L))
    }

    @Test
    fun `test game details`() {
        catalog.queryGameDetails(Game.Id("43663/007-tomorrow-never-dies-ps1")).run {
            assertEquals(null, chiptunePath)
            assertEquals(
                "images/games/ps1/3/007-tomorrow-never-dies-ps1-title-81143.png", image?.value
            )
        }
        catalog.queryGameDetails(Game.Id("501/1943-the-battle-of-midway-nes")).run {
            assertEquals(
                FilePath("music/chiptunes/archives/0/1943-the-battle-of-midway-nes-[NSF-ID1757].nsf"),
                chiptunePath
            )
            assertEquals(
                "images/games/nes/1/1943-the-battle-of-midway-nes-title-59899.jpg", image?.value
            )
        }
    }

    @Test
    fun `test album image`() {
        assertEquals(
            FilePath("images/albums/0/5/30-105.jpg"),
            catalog.queryAlbumImage(Album.Id("30/nights-lucid-dreaming"))
        )
    }

    @Test
    fun `test invalid objects`() {
        val sys = Catalog.SystemScope(System.Id("invalid"))
        val org = Catalog.OrganizationScope(Organization.Id("0/invalid"))
        for (scope in arrayOf(
            sys, Catalog.GameScope(Game.Id("0/invalid")), org
        )) {
            assertThrows<FileNotFoundException> {
                catalog.queryAlbums(scope, albumsVisitor, progress)
            }
            assertThrows<FileNotFoundException> {
                catalog.queryRemixes(scope, remixesVisitor, progress)
            }
        }
        for (scope in arrayOf(sys, org)) {
            assertThrows<FileNotFoundException> {
                catalog.queryGames(scope, gamesVisitor, progress)
            }
        }
        assertThrows<FileNotFoundException> {
            catalog.findRemixPath(Remix.Id("OCR0"))
        }
    }

    @Test
    fun `test remote urls`() {
        // Really files don't contain symbols require encoding
        RemoteCatalog.getRemoteUris(FilePath("music/remixes/Remix file.mp3")).run {
            assertEquals(2, size)
            assertEquals(
                "${BuildConfig.CDN_ROOT}/download/ocremix/files/music/remixes/Remix%20file.mp3",
                get(0).toString()
            )
            assertEquals(
                "https://ocrmirror.org/files/music/remixes/Remix%20file.mp3", get(1).toString()
            )
        }
        RemoteCatalog.getRemoteUris(FilePath("music/chiptunes/archives/0/Some.rsn")).run {
            assertEquals(2, size)
            assertEquals(
                "${BuildConfig.CDN_ROOT}/download/ocremix/files/music/chiptunes/archives/0/Some.rsn",
                get(0).toString()
            )
            assertEquals(
                "https://ocrmirror.org/files/music/chiptunes/archives/0/Some.rsn", get(1).toString()
            )
        }
        RemoteCatalog.getRemoteUris(FilePath("torrents/some.torrent")).run {
            assertEquals(2, size)
            assertEquals(
                "${BuildConfig.CDN_ROOT}/download/ocremix/files/torrents/some.torrent",
                get(0).toString()
            )
            assertEquals("https://bt.ocremix.org/torrents/some.torrent", get(1).toString())
        }
        RemoteCatalog.getRemoteUris(FilePath("albums/Some Album/Track.mp3")).run {
            assertEquals(1, size)
            assertEquals(
                "${BuildConfig.CDN_ROOT}/download/ocremix/files/albums/Some%20Album/Track.mp3",
                get(0).toString()
            )
        }
    }

    companion object {
        fun sys(id: String, title: String) = System(System.Id(id), title)
        fun org(id: String, title: String) = Organization(Organization.Id(id), title)
        fun game(id: String, title: String) = Game(Game.Id(id), title)
        fun remix(id: String, title: String) = Remix(Remix.Id(id), title)
        fun album(id: String, title: String) = Album(Album.Id(id), title)
    }
}
