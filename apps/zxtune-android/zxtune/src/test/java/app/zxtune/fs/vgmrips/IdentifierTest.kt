package app.zxtune.fs.vgmrips

import android.net.Uri
import app.zxtune.utils.ifNotNulls
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
            group = makeGroup("sega", "Sega")
        )
        testParse(
            "vgmrips:/Composer/Shimomura/Street%20Fighter?group=shimomura&pack=sf&location=loc",
            category = Identifier.CATEGORY_COMPOSER,
            group = makeGroup("shimomura", "Shimomura"),
            pack = Pack(Pack.Id("sf"), "Street Fighter", FilePath("loc"))
        )
    }

    @Test
    fun `test compatibility`() {
        testAnalyze(
            "vgmrips:/Chip/AY8910/CSCDejaVu/intro.vgz?group=ay8910&pack=cscdjvu&track=ZX%2FCSC%2Fintro.vgz",
            category = Identifier.CATEGORY_CHIP,
            group = makeGroup("ay8910", "AY8910"),
            packId = Pack.Id("cscdjvu"),
            track = FilePath("ZX/CSC/intro.vgz")
        )
        testAnalyze(
            "vgmrips:/Random/Pak/trak.vgz?pack=pak&track=dir%2Ffile.vgz",
            category = Identifier.CATEGORY_RANDOM,
            packId = Pack.Id("pak"),
            track = FilePath("dir/file.vgz")
        )
    }

    @Test
    fun `test random`() {
        testParse("vgmrips:/Random", category = Identifier.CATEGORY_RANDOM)
        testParse(
            "vgmrips:/Random?pack=pak&track=path%2Ftrack.vgz",
            category = Identifier.CATEGORY_RANDOM,
            packId = Pack.Id("pak"),
            track = FilePath("path/track.vgz")
        )
        testAnalyze(
            "vgmrips:/Random/Pck?pack=pck",
            category = Identifier.CATEGORY_RANDOM,
            packId = Pack.Id("pck")
        )
    }

    @Test
    fun `test images`() {
        testParseImage("vgmrips:/Image")
        testParseImage(
            "vgmrips:/Image?location=image%2Flocation", image = FilePath("image/location")
        )
    }

    @Test
    fun `test errors`() {
        // Scheme validity is checked at another level
        testAnalyze(
            "vgm:/Chips/Lays/Packed/riffle?group=gr&pack=pk&track=trk",
            isFromRoot = false,
            category = "Chips",
            group = makeGroup("gr", "Lays"),
            packId = Pack.Id("pk"),
            track = FilePath("trk")
        )
        // track filename is the last path component
        testAnalyze(
            "vgm:/G/R/O/U/P/track.vgz?track=tr",
            isFromRoot = false,
            category = "G",
        )
    }
}

private fun testParse(
    uri: String,
    category: String? = null,
    group: Group? = null,
    pack: Pack? = null,
    packId: Pack.Id? = pack?.id,
    track: FilePath? = null
) {
    testAnalyze(
        uri = uri,
        isFromRoot = true,
        category = category,
        group = group,
        pack = pack,
        packId = packId,
        track = track
    )
    if (packId != null && track != null) {
        assertEquals("uri", uri, Identifier.forRandomTrack(packId, track).toString())
        return
    }
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
    assertEquals("uri", uri, builder.toString())
}

private fun makeGroup(id: String, title: String) = Group(Group.Id(id), title, 0)

private fun testAnalyze(
    uri: String,
    isFromRoot: Boolean = true,
    category: String? = null,
    group: Group? = null,
    pack: Pack? = null,
    packId: Pack.Id? = pack?.id,
    track: FilePath? = null
) = Uri.parse(uri).let { url ->
    val path = url.pathSegments
    assertEquals("isFromRoot", isFromRoot, Identifier.isFromRoot(url))
    assertEquals("findCategory", category, Identifier.findCategory(path))
    assertEquals("findGroup", group, Identifier.findGroup(url, path))
    assertEquals("findPack", pack, Identifier.findPack(url, path))
    assertEquals("findPackId", packId, Identifier.findPackId(url))
    assertEquals("findTrack", ifNotNulls(packId, track, ::Pair), Identifier.findTrack(url))
}

private fun testParseImage(
    uri: String,
    image: FilePath? = null,
) {
    testAnalyzeImages(uri, image = image)
    image?.let {
        assertEquals("uri", uri, Identifier.forImage(it).toString())
    }
}

private fun testAnalyzeImages(uri: String, image: FilePath? = null) = Uri.parse(uri).let { url ->
    val path = url.pathSegments
    assertEquals("isFromRoot", true, Identifier.isFromRoot(url))
    assertEquals("findCategory", Identifier.CATEGORY_IMAGE, Identifier.findCategory(path))
    assertEquals("findPackForImage", image, Identifier.findImage(url))
}
