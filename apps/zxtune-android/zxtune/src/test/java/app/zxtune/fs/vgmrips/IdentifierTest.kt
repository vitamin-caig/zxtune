package app.zxtune.fs.vgmrips

import android.net.Uri
import app.zxtune.TimeStamp
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class IdentifierTest {

    @Test
    fun `test root`() {
        testParse("vgmrips:")
    }

    @Test
    fun `test all`() {
        testParse("vgmrips:/System", category = Identifier.CATEGORY_SYSTEM)
        testParse(
            "vgmrips:/Company/Sega?group=sega",
            category = Identifier.CATEGORY_COMPANY,
            group = Group("sega", "Sega", 0)
        )
        testParse(
            "vgmrips:/Composer/Shimomura/Street%20Fighter?group=shimomura&pack=sf",
            category = Identifier.CATEGORY_COMPOSER,
            group = Group("shimomura", "Shimomura", 0),
            pack = Pack("sf", "Street Fighter")
        )
        testParse(
            "vgmrips:/Chip/AY8910/CSCDejaVu/intro.vgz?group=ay8910&pack=cscdjvu&track=ZX%2FCSC%2Fintro.vgz",
            category = Identifier.CATEGORY_CHIP,
            group = Group("ay8910", "AY8910", 0),
            pack = Pack("cscdjvu", "CSCDejaVu"),
            track = Track(0, "intro.vgz", TimeStamp.EMPTY, "ZX/CSC/intro.vgz")
        )
    }

    @Test
    fun `test random`() {
        testParse("vgmrips:/Random", category = Identifier.CATEGORY_RANDOM)
        // special parsing rules
        "vgmrips:/Random/Pck?pack=pck".let {
            testAnalyze(it, category = Identifier.CATEGORY_RANDOM)
            Uri.parse(it).let { url ->
                val path = url.pathSegments
                assertEquals(
                    "findRandomPack",
                    Pack("pck", "Pck"),
                    Identifier.findRandomPack(url, path)
                )
            }
        }
        "vgmrips:/Random/Pak/trak.vgz?pack=pak&track=dir%2Ffile.vgz".let {
            testAnalyze(
                it,
                category = Identifier.CATEGORY_RANDOM,
                pack = Pack("pak", "trak.vgz"), // <- intented to be wrong
                track = Track(0, "trak.vgz", TimeStamp.EMPTY, "dir/file.vgz")
            )
            Uri.parse(it).let { url ->
                val path = url.pathSegments
                assertEquals(
                    "findRandomPack",
                    Pack("pak", "Pak"),
                    Identifier.findRandomPack(url, path)
                )
            }
        }
    }

    @Test
    fun `test errors`() {
        // Scheme validity is checked at another level
        testAnalyze(
            "vgm:/Chips/Lays/Packed/riffle?group=gr&pack=pk&track=trk",
            isFromRoot = false,
            category = "Chips",
            group = Group("gr", "Lays", 0),
            pack = Pack("pk", "Packed"),
            track = Track(0, "riffle", TimeStamp.EMPTY, "trk")
        )
        // track filename is the last path component
        testAnalyze(
            "vgm:/G/R/O/U/P/track.vgz?track=tr",
            isFromRoot = false,
            category = "G",
            track = Track(0, "track.vgz", TimeStamp.EMPTY, "tr")
        )
    }
}

private fun testParse(
    uri: String,
    category: String? = null,
    group: Group? = null,
    pack: Pack? = null,
    track: Track? = null
) {
    testAnalyze(
        uri = uri,
        isFromRoot = true,
        category = category,
        group = group,
        pack = pack,
        track = track
    )
    var builder = when {
        category != null -> Identifier.forCategory(category)
        else -> Identifier.forRoot()
    }
    if (group != null) {
        builder = Identifier.forGroup(builder, group)
    }
    if (pack != null) {
        builder = Identifier.forPack(builder, pack)
    }
    if (track != null) {
        builder = Identifier.forTrack(builder, track)
    }
    assertEquals("uri", uri, builder.toString())
}

private fun testAnalyze(
    uri: String,
    isFromRoot: Boolean = true,
    category: String? = null,
    group: Group? = null,
    pack: Pack? = null,
    track: Track? = null
) = Uri.parse(uri).let { url ->
    val path = url.pathSegments
    assertEquals("isFromRoot", isFromRoot, Identifier.isFromRoot(url))
    assertEquals("findCategory", category, Identifier.findCategory(path))
    assertEquals("findGroup", group, Identifier.findGroup(url, path))
    assertEquals("findPack", pack, Identifier.findPack(url, path))
    assertEquals("findTrack", track, Identifier.findTrack(url, path))
}
