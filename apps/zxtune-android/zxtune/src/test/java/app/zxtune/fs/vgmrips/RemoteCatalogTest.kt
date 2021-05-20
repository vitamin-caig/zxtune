package app.zxtune.fs.vgmrips

import app.zxtune.BuildConfig
import app.zxtune.TimeStamp
import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class RemoteCatalogTest {

    private lateinit var catalog: RemoteCatalog

    @Before
    fun setUp() = with(HttpProviderFactory.createTestProvider()) {
        catalog = RemoteCatalog(MultisourceHttpProvider(this))
    }

    @Test
    fun `test companies`() {
        val checkpoints = arrayOf( //first
            Group("konami", "Konami", 219),
            Group("naxat-soft", "Naxat Soft", 5),
            Group("c-s-ware", "C's Ware", 4),
            Group("crea-tech", "Crea-Tech", 1)
        )
        with(GroupsChecker()) {
            catalog.companies().query(this)
            check(555, checkpoints)
        }
    }

    @Test
    fun `test company packs`() {
        val checkpoints = arrayOf( // first
            makePack("solitaire-nes", "Solitaire", 4, 25, 3)
        )
        with(PacksChecker()) {
            catalog.companies().queryPacks("odyssey-software", this, this)
            check(4, checkpoints)
            checkProgress(4, 4)
        }
    }

    @Test
    fun `test composers`() {
        val checkpoints = arrayOf( //first
            Group("konami-kukeiha-club", "Konami Kukeiha Club", 61),
            Group("mariko-egawa", "Mariko Egawa", 2),  //last
            Group("zuntata-sound-team", "Zuntata Sound Team", 1)
        )
        with(GroupsChecker()) {
            catalog.composers().query(this)
            check(1541, checkpoints)
        }
    }

    @Test
    fun `test composer packs`() {
        val checkpoints = arrayOf( // first
            makePack(
                "street-fighter-ii-champion-edition-cp-system",
                "Street Fighter II: Champion Edition",
                47,
                45,
                13
            )
        )
        with(PacksChecker()) {
            catalog.composers().queryPacks("yoko-shimomura", this, this)
            check(15, checkpoints)
            checkProgress(15, 15)
        }
    }

    @Test
    fun `test chips`() {
        val checkpoints = arrayOf( //first
            Group("nes-apu", "NES APU", 437),  //last
            Group("vrc7", "VRC7", 1)
        )
        with(GroupsChecker()) {
            catalog.chips().query(this)
            check(51, checkpoints)
        }
    }

    @Test
    fun `test chip packs`() {
        val checkpoints = arrayOf( //first
            makePack(
                "sound-blaster-series-demo-songs-ibm-pc-xt-at",
                "Sound Blaster Series Demo Songs",
                17,
                30,
                4
            )
        )
        with(PacksChecker()) {
            catalog.chips().queryPacks("saa1099", this, this)
            check(4, checkpoints)
            checkProgress(4, 4)
        }
    }

    @Test
    fun `test systems`() {
        val checkpoins = arrayOf( //first
            Group(
                "nintendo/family-computer", "Family Computer",
                333
            ),
            Group("ascii/msx", "MSX", 89) //last
            //checkpoins.append(182, new System("snk/neo-geo-pocket", "Neo Geo Pocket", 1));
        )
        with(GroupsChecker()) {
            catalog.systems().query(this)
            check(186, checkpoins)
        }
    }

    @Test
    fun `test system packs`() {
        val checkpoints = arrayOf( //first
            makePack(
                "the-ninja-warriors-zx-spectrum-128", "The Ninja Warriors",
                1, 45, 8
            ),  //last
            makePack(
                "altered-beast-zx-spectrum-128", "Altered Beast",
                5, 15, 4
            )
        )
        with(PacksChecker()) {
            catalog.systems().queryPacks("sinclair/zx-spectrum-128", this, this)
            check(32, checkpoints)
            checkProgress(32, 32)
        }
    }

    @Test
    fun `test pack tracks`() {
        val checkpoints = arrayOf( //first
            Track(
                1,
                "Into The Lair",
                TimeStamp.fromSeconds(54),
                "Other/The_Scheme_(NEC_PC-8801,_OPNA)/01 Into The Lair.vgz"
            ),  //last
            Track(
                17,
                "Theme of Gigaikotsu",
                TimeStamp.fromSeconds(121),
                "Other/The_Scheme_(NEC_PC-8801,_OPNA)/17 Theme of Gigaikotsu.vgz"
            )
        )
        with(TracksChecker()) {
            val pack = catalog.findPack("the-scheme-nec-pc-8801-opna", this)
            assertTrue(
                matches(
                    makePack("the-scheme-nec-pc-8801-opna", "The Scheme", 17, 45, 34),
                    pack
                )
            )
            check(17, checkpoints)
        }
    }

    @Test
    fun `test getTrackUris`() = with(
        RemoteCatalog.getRemoteUris(
            Track(
                123,
                "Unused",
                TimeStamp.EMPTY,
                "track/location/file.gz"
            )
        )
    ) {
        assertEquals(2L, size.toLong())
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/vgmrips/track/location/file.gz",
            get(0).toString()
        )
        assertEquals(
            "https://vgmrips.net/packs/vgm/track/location/file.gz",
            get(1).toString()
        )
    }

    private class GroupsChecker : Catalog.Visitor<Group> {

        val result = ArrayList<Group>()

        override fun accept(obj: Group) {
            assertEquals(obj.id, obj.id.trim())
            assertEquals(obj.title, obj.title.trim())
            assertNotEquals(0, obj.packs)
            result.add(obj)
        }

        fun check(size: Int, checkpoints: Array<Group>) {
            check(size, checkpoints, result)
        }
    }

    private class PacksChecker : Catalog.Visitor<Pack>, ProgressCallback {

        val result = ArrayList<Pack>()
        val lastProgress = intArrayOf(-1, -1)

        override fun accept(obj: Pack) {
            assertEquals(obj.id, obj.id.trim())
            assertEquals(obj.title, obj.title.trim())
            result.add(obj)
        }

        fun check(size: Int, checkpoints: Array<Pack>) {
            check(size, checkpoints, result)
        }

        override fun onProgressUpdate(done: Int, total: Int) {
            lastProgress[0] = done
            lastProgress[1] = total
        }

        fun checkProgress(done: Int, total: Int) {
            assertEquals(done, lastProgress[0])
            assertEquals(total, lastProgress[1])
        }
    }

    private class TracksChecker : Catalog.Visitor<Track> {

        val result = ArrayList<Track>()

        override fun accept(obj: Track) {
            assertEquals(obj.title, obj.title.trim())
            assertEquals(obj.location, obj.location.trim())
            result.add(obj)
        }

        fun check(size: Int, checkpoints: Array<Track>) {
            check(size, checkpoints, result)
        }
    }
}

private fun <T> check(minSize: Int, expectedSubset: Array<T>, actual: ArrayList<T>) {
    assertTrue(actual.size >= minSize)
    for (expected in expectedSubset) {
        assertTrue("Not found $expected", contains(actual, expected))
    }
}

private fun <T> contains(actual: ArrayList<T>, expected: T) =
    actual.find { matches(expected, it) } != null

private fun <T> matches(expected: T, actual: T) = when (expected) {
    is Group -> matches(expected, actual as Group)
    is Pack -> matches(expected, actual as Pack)
    is Track -> matches(expected, actual as Track)
    else -> {
        fail("Unknown type $expected")
        false
    }
}

// Match only visible in UI attributes
private fun matches(expected: Group, actual: Group) =
    if (expected.id == actual.id) {
        assertEquals(expected.id, expected.title, actual.title)
        assertTrue(expected.id, actual.packs >= expected.packs)
        true
    } else {
        false
    }

private fun matches(expected: Pack, actual: Pack?) =
    if (expected.id == actual!!.id) {
        assertEquals(expected.id, expected.title, actual.title)
        //assertEquals(expected.id, expected.ratings, actual.ratings);
        //assertEquals(expected.id, expected.score, actual.score);
        assertEquals(expected.id, expected.songs.toLong(), actual.songs.toLong())
        true
    } else {
        false
    }

private fun matches(expected: Track, actual: Track) =
    if (expected.location == actual.location) {
        assertEquals(expected.location, expected.title, actual.title)
        assertEquals(expected.location, expected.number, actual.number)
        assertEquals(expected.location, expected.duration, actual.duration)
        true
    } else {
        false
    }

private fun makePack(id: String, title: String, songs: Int, score: Int, ratings: Int) =
    Pack(id, title).apply {
        this.songs = songs
        this.score = score
        this.ratings = ratings
    }
