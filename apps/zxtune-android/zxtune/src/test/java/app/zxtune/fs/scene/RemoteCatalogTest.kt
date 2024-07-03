package app.zxtune.fs.scene

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
            "artists", "",
            "compos", "",
            "disks", "",
            "groups", ""
        )
        test(this, entries, Mode.CHECK_MISSED)
    }

    @Test
    fun `test listing`() = with(Path.parse(Uri.parse("scene:/groups/farb-rausch"))!!) {
        val entries = arrayOf(
            "005-mentz-last_shadow.mp3", "7.2M",
            "018-rp-axodry.mp3", if (remoteUrlIdx == 0) "3.6M" else "3.5M",
            "wayfinder-fr-025_(single_edit).mp3", "4.2M"
        )
        test(this, entries, Mode.CHECK_MISSED)
    }
}
