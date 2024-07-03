package app.zxtune.fs.aygor

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
        val entries = arrayOf(
            "DualCore", "",
            "games", "",
            "musicians", "",
            "scene", ""
        )
        test(this, entries)
    }

    @Test
    fun `test truncate`() = with(Path.parse(Uri.parse("aygor:/musicians/A/AS"))!!) {
        val entries = arrayOf(
            "Prince_of_Persia_intro.ay", "16K"
        )
        test(this, entries)
    }

    @Test
    fun `test suspicious files`() = with(Path.parse(Uri.parse("aygor:/scene/scene_cpc"))!!) {
        val entries = arrayOf( //not an .ay
            "!info.txt", "36",  //dir
            "Audio_Illusions", "",  //truncated
            "CPC_Telegramm_16[CPC].ay", "8.6K",
            "CPC_Telegramm_17[CPC].ay", "9.5K"
        )
        test(this, entries, Mode.CHECK_MISSED)
    }
}
