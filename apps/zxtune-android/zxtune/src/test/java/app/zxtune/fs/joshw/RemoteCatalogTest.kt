package app.zxtune.fs.joshw

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
    fun `test KSS root`() = with(Path.parse(Uri.parse("joshw:/kss"))!!) {
        val entries = arrayOf(
            "Game Gear", "",
            "MSX", "",
            "Master System", ""
        )
        test(this, entries)
    }

    @Test
    fun `test NSF n`() = with(Path.parse(Uri.parse("joshw:/nsf/n"))!!) {
        val entries = arrayOf( //first
            "NARC (1990-08)(Williams)(Rare)(Acclaim)[NES].7z", "3.3K",  //escaped
            "North & South (1990-09-21)(Kemco)[NES].7z", "10.6K",  //11K for native
            //last
            "Nuts & Milk (1984-07-28)(Hudson)[NES].7z", "3.0K"
        )
        test(this, entries, Mode.CHECK_MISSED)
    }

    @Test
    fun `test big file`() = with(Path.parse(Uri.parse("joshw:/pc/t"))!!) {
        val entries = arrayOf(
            "Tekken 7 (2017-06-02)(-)(Bandai Namco)[PC].7z", "3.4G"
        )
        test(this, entries, Mode.CHECK_MISSED)
    }
}
