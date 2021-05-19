package app.zxtune.fs.hvsc

import android.net.Uri
import app.zxtune.fs.httpdir.RemoteCatalogTestBase
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class CatalogTest : RemoteCatalogTestBase() {

    @Before
    fun setUp() = setUpTest()

    @Test
    fun `test root`() = with(Path.create()) {
        //hardcoded entries, filtered in Root
        val entries = arrayOf(
            "DEMOS", "",
            "GAMES", "",
            "MUSICIANS", ""
        )
        test(this, entries, Mode.CHECK_MISSED)
    }

    @Test
    fun `test listing`() = with(Path.parse(Uri.parse("hvsc:/MUSICIANS/O/OPM"))!!) {
        val entries = arrayOf(
            "Fantjes_Beat.sid", "1.9K",
            "Rockmonitor_3.sid", "35.1K",  //35K for native
            "Rockmonitor_5_Demosong.sid", "18.3K",  //18K for native
            "Sid_Slam.sid", "1.8K",
            "Zoolook.sid", "3.2K"
        )
        test(this, entries)
    }
}
