package app.zxtune.fs.scene

import android.net.Uri
import app.zxtune.fs.httpdir.RemoteCatalogTestBase
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class RemoteCatalogTest : RemoteCatalogTestBase() {

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
            "018-rp-axodry.mp3", "3.6M",
            "wayfinder-fr-025_(single_edit).mp3", "4.2M"
        )
        test(this, entries, Mode.CHECK_MISSED)
    }
}
