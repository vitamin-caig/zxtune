package app.zxtune.fs.api

import app.zxtune.BuildConfig
import app.zxtune.assertThrows
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import kotlin.IllegalArgumentException

@RunWith(RobolectricTestRunner::class)
class CdnTest {

    @Test
    fun `test ASMA`() {
        assertEquals("${BuildConfig.CDN_ROOT}/browse/asma/", Cdn.asma("").toString())
        assertEquals(
            "${BuildConfig.CDN_ROOT}/browse/asma/Unknown/AMP/A_Bottle_of_Wine.sap",
            Cdn.asma("Unknown/AMP/A_Bottle_of_Wine.sap").toString()
        )
    }

    @Test
    fun `test HVSC`() {
        assertEquals("${BuildConfig.CDN_ROOT}/browse/hvsc/", Cdn.hvsc("").toString())
        assertEquals(
            "${BuildConfig.CDN_ROOT}/browse/hvsc/MUSICIANS/0-9/451/A_Letter_from_the_Quarantine_Directory.sid",
            Cdn.hvsc("MUSICIANS/0-9/451/A_Letter_from_the_Quarantine_Directory.sid").toString()
        )
    }

    @Test
    fun `test JoshW`() {
        assertEquals("${BuildConfig.CDN_ROOT}/browse/joshw/pc/", Cdn.joshw("pc", "").toString())
        assertEquals(
            "${BuildConfig.CDN_ROOT}/browse/joshw/s98/m/Markadia%20(PC-98)(19xx)(Bio%20100%25).7z",
            Cdn.joshw("s98", "m/Markadia (PC-98)(19xx)(Bio 100%).7z").toString()
        )
        assertEquals(
            "${BuildConfig.CDN_ROOT}/browse/joshw/mobile/m/Ms.%20%60Splosion%20Man%20(2013-03-28)(Twisted%20Pixel)(Microsoft)%5BiOS%5D.7z",
            Cdn.joshw(
                "mobile",
                "m/Ms. `Splosion Man (2013-03-28)(Twisted Pixel)(Microsoft)[iOS].7z"
            ).toString()
        )
        assertEquals(
            "${BuildConfig.CDN_ROOT}/browse/joshw/gsf/k/Kid's%20Cards%20(2006-06-15)(Skyworks)(Majesco)%5BGBA%5D.7z",
            Cdn.joshw("gsf", "k/Kid's Cards (2006-06-15)(Skyworks)(Majesco)[GBA].7z").toString()
        )
    }

    @Test
    fun `test AMP`() {
        assertEquals("${BuildConfig.CDN_ROOT}/download/amp/ids/12345", Cdn.amp(12345).toString())
    }

    @Test
    fun `test ModLand`() {
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/modland/pub/modules/Fasttracker%202/4-Mat/blade1.xm",
            Cdn.modland("/pub/modules/Fasttracker%202/4-Mat/blade1.xm").toString()
        )
    }

    @Test
    fun `test ModArchive`() {
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/modarchive/ids/23456",
            Cdn.modarchive(23456).toString()
        )
    }

    @Test
    fun `test AmiNet`() {
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/aminet/mods/",
            Cdn.aminet("").toString()
        )
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/aminet/mods/4mat/4mat_1.lha",
            Cdn.aminet("4mat/4mat_1.lha").toString()
        )
    }

    @Test
    fun `test VgmRips`() {
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/vgmrips/Other/The_Scheme_(NEC_PC-8801%2C_OPNA)/17%20Theme%20of%20Gigaikotsu.vgz",
            Cdn.vgmrips("Other/The_Scheme_(NEC_PC-8801,_OPNA)/17 Theme of Gigaikotsu.vgz")
                .toString()
        )
    }

    @Test
    fun `test Scene`() {
        assertEquals(
            "${BuildConfig.CDN_ROOT}/browse/scene/music/",
            Cdn.scene("").toString()
        )
        assertEquals(
            "${BuildConfig.CDN_ROOT}/browse/scene/music/artists/jazz/antigotchi%20remix%202003.mp3",
            Cdn.scene("artists/jazz/antigotchi remix 2003.mp3").toString()
        )
        assertEquals(
            "${BuildConfig.CDN_ROOT}/browse/scene/music/groups/flange/2001/alex_garcia_(alexg)" +
                    "_%26_jamie_holyoake_(mr.d)-flange_-%60flight_(resampled_drums).mp3",
            Cdn.scene(
                "groups/flange/2001/alex_garcia_(alexg)_&_jamie_holyoake_(mr.d)" +
                        "-flange_-`flight_(resampled_drums).mp3"
            ).toString()
        )
    }

    @Test
    fun `test ocremix`() {
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/ocremix/files/some/file%20name",
            Cdn.ocremix("files/some/file name").toString()
        )
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/ocremix/img-size/100/files/some/file%20name",
            Cdn.ocremix("img-size/100/files/some/file name").toString()
        )
        assertThrows<IllegalArgumentException> {
            Cdn.ocremix("invalid/path")
        }
    }
}
