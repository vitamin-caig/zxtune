package app.zxtune.fs.hvsc

import android.net.Uri
import app.zxtune.fs.httpdir.RemoteCatalogTestBase
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.ParameterizedRobolectricTestRunner

@RunWith(ParameterizedRobolectricTestRunner::class)
class RemoteCatalogTest(remoteUrlIdx: Int) : RemoteCatalogTestBase(remoteUrlIdx) {

    companion object {
        @JvmStatic
        @ParameterizedRobolectricTestRunner.Parameters
        fun data() = Array(Path.REMOTE_URLS_COUNT) { i -> i }
    }

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
            "Rockmonitor_3.sid", if (remoteUrlIdx == 0) "35.1K" else "35K",
            "Rockmonitor_5_Demosong.sid", if (remoteUrlIdx == 0) "18.3K" else "18K",
            "Sid_Slam.sid", "1.8K",
            "Zoolook.sid", "3.2K"
        )
        test(this, entries)
    }
}
