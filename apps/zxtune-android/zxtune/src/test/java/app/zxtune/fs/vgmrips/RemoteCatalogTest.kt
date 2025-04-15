package app.zxtune.fs.vgmrips

import app.zxtune.BuildConfig
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
import org.mockito.kotlin.argThat
import org.mockito.kotlin.atLeast
import org.mockito.kotlin.mock
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class RemoteCatalogTest {

    private lateinit var catalog: RemoteCatalog
    private lateinit var groupsVisitor: Catalog.Visitor<Group>
    private lateinit var packsVisitor: Catalog.Visitor<Pack>
    private lateinit var tracksVisitor: Catalog.Visitor<FilePath>
    private lateinit var progress: ProgressCallback

    @Before
    fun setUp() {
        catalog = RemoteCatalog(MultisourceHttpProvider(HttpProviderFactory.createTestProvider()))
        groupsVisitor = mock {}
        packsVisitor = mock {}
        tracksVisitor = mock {}
        progress = mock {}
    }

    @After
    fun tearDown() = verifyNoMoreInteractions(groupsVisitor, packsVisitor, progress)

    @Test
    fun `test companies`() = checkGroups(
        catalog.companies(),
        groupsMin = 555,
        makeGroup("konami", "Konami", 219),
        makeGroup("naxat-soft", "Naxat Soft", 5),
        makeGroup("c-s-ware", "C's Ware", 4),
        makeGroup("crea-tech", "Crea-Tech", 1)
    )

    @Test
    fun `test company packs`() = checkPacks(
        catalog.companies(), "odyssey-software", packsMin = 4, pagesMin = 1, makePack(
            "solitaire-nes",
            "Solitaire",
            4,
            "13.5 KB",
            "NES/Solitaire_(NES).zip",
            "NES/Solitaire_(NES).png"
        )
    )

    @Test
    fun `test composers`() = checkGroups(
        catalog.composers(),
        groupsMin = 1540,
        makeGroup("konami-kukeiha-club", "Konami Kukeiha Club", 61),
        makeGroup("mariko-egawa", "Mariko Egawa", 2),
        makeGroup("zuntata-sound-team", "Zuntata Sound Team", 1)
    )

    @Test
    fun `test composer packs`() = checkPacks(
        catalog.composers(), "yoko-shimomura", packsMin = 18, pagesMin = 1, makePack(
            "street-fighter-ii-champion-edition-cp-system",
            "Street Fighter II: Champion Edition",
            47,
            "823.14 KB",
            "Arcade/Capcom/Street_Fighter_II_-_Champion_Edition_(CP_System).zip",
            "Arcade/Capcom/Street_Fighter_II_-_Champion_Edition_(CP_System).png"
        )
    )

    @Test
    fun `test chips`() =
        checkGroups(catalog.chips(), groupsMin = 50, makeGroup("nes-apu", "NES APU", 437))

    @Test
    fun `test chip packs`() = checkPacks(
        catalog.chips(), "saa1099", packsMin = 4, pagesMin = 1, makePack(
            "sound-blaster-series-demo-songs-ibm-pc-xt-at",
            "Sound Blaster Series Demo Songs",
            17,
            "102.26 KB",
            "Computers/IBM_PC/Sound_Blaster_Series_Demo_Songs_(IBM_PC_XT_AT).zip",
            "Computers/IBM_PC/Sound_Blaster_Series_Demo_Songs_(IBM_PC_XT_AT)_D1.png"
        )
    )

    @Test
    fun `test systems`() = checkGroups(
        catalog.systems(),
        groupsMin = 180,
        makeGroup("nintendo/family-computer", "Family Computer", 333),
        makeGroup("ascii/msx", "MSX", 89),
        makeGroup("snk/neo-geo-pocket", "Neo Geo Pocket", 1)
    )

    @Test
    fun `test system packs`() = checkPacks(
        catalog.systems(), "sinclair/zx-spectrum-128", packsMin = 34, pagesMin = 2, makePack(
            "the-ninja-warriors-zx-spectrum-128",
            "The Ninja Warriors",
            1,
            "68.61 KB",
            "Computers/ZX_Spectrum/The_Ninja_Warriors_(ZX_Spectrum_128).zip",
            "Computers/ZX_Spectrum/The_Ninja_Warriors_(ZX_Spectrum_128).png"
        ), makePack(
            "altered-beast-zx-spectrum-128",
            "Altered Beast",
            5,
            "42.52 KB",
            "Computers/ZX_Spectrum/Altered_Beast_(ZX_Spectrum_128).zip",
            "Computers/ZX_Spectrum/Altered_Beast_(ZX_Spectrum_128).png"
        )
    )

    @Test
    fun `test pack tracks`() {
        val pack =
            catalog.findPackInternal("/packs/pack/the-scheme-nec-pc-8801-opna", tracksVisitor)
        val tracksMin = 17
        verify(tracksVisitor).accept(
            FilePath("Computers/NEC/The_Scheme_(NEC_PC-8801,_OPNA)/01 Into The Lair.vgz")
        )
        verify(tracksVisitor).accept(
            FilePath("Computers/NEC/The_Scheme_(NEC_PC-8801,_OPNA)/17 Theme of Gigaikotsu.vgz")
        )
        verify(tracksVisitor, atLeast(tracksMin)).accept(any())
        // To have detailed log if something mismatched
        packsVisitor.accept(requireNotNull(pack))
        verify(packsVisitor).accept(argThat {
            matches(
                makePack(
                    "the-scheme-nec-pc-8801-opna",
                    "The Scheme",
                    0,
                    "",
                    "Computers/NEC/The_Scheme_(NEC_PC-8801,_OPNA).zip",
                    "Computers/NEC/The_Scheme_(NEC_PC-8801,_OPNA).png"
                )
            )
        })
    }

    @Test
    fun `test getPackUris`() = with(
        RemoteCatalog.getRemoteUris(FilePath("pack/file.zip"))
    ) {
        assertEquals(2L, size.toLong())
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/vgmrips/files/pack/file.zip", get(0).toString()
        )
        assertEquals(
            "https://vgmrips.net/files/pack/file.zip", get(1).toString()
        )
    }

    @Test
    fun `test getTrackUris`() = with(
        RemoteCatalog.getRemoteUris(FilePath("track/location/file.vgz"))
    ) {
        assertEquals(2L, size.toLong())
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/vgmrips/track/location/file.vgz", get(0).toString()
        )
        assertEquals(
            "https://vgmrips.net/packs/vgm/track/location/file.vgz", get(1).toString()
        )
    }

    @Test
    fun `test getImageUris`() = with(
        RemoteCatalog.getRemoteUris(FilePath("pack/file.PNG"))
    ) {
        assertEquals(2L, size.toLong())
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/vgmrips/packs/images/large/pack/file.PNG",
            get(0).toString()
        )
        assertEquals(
            "https://vgmrips.net/packs/images/large/pack/file.PNG", get(1).toString()
        )
    }

    private fun checkGroups(grouping: Catalog.Grouping, groupsMin: Int, vararg groups: Group) {
        grouping.query(groupsVisitor)

        groups.forEach { expected ->
            verify(groupsVisitor).accept(argThat {
                id == expected.id && title == expected.title && packs >= expected.packs
            })
        }
        verify(groupsVisitor, atLeast(groupsMin)).accept(any())
    }

    private fun checkPacks(
        grouping: Catalog.Grouping,
        groupId: String,
        packsMin: Int,
        pagesMin: Int,
        vararg packs: Pack
    ) {
        grouping.queryPacks(Group.Id(groupId), packsVisitor, progress)

        packs.forEach { expected ->
            verify(packsVisitor).accept(argThat { matches(expected) })
        }
        verify(packsVisitor, atLeast(packsMin)).accept(any())
        verify(progress, atLeast(pagesMin)).onProgressUpdate(any(), geq(packsMin))
    }
}

private fun Pack.matches(expected: Pack) =
    id == expected.id && title == expected.title && songs == expected.songs && size == expected.size && archive == expected.archive && image == expected.image

private fun makeGroup(id: String, title: String, packs: Int) = Group(Group.Id(id), title, packs)

private fun makePack(
    id: String, title: String, songs: Int, size: String, archive: String, image: String
) = Pack(Pack.Id(id), title, FilePath(archive), FilePath(image), songs, size)
