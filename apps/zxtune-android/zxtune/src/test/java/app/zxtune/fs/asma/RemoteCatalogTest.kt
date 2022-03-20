package app.zxtune.fs.asma

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
        //hardcoded entries, filtered in Root
        val entries = arrayOf(
            "Composers", "",
            "Games", "",
            "Misc", "",
            "Unknown", ""
        )
        test(this, entries, Mode.CHECK_MISSED)
    }

    @Test
    fun `test listing`() = with(Path.parse(Uri.parse("asma:/Composers/Bendyk_Piotr"))!!) {
        val entries = arrayOf(
            "Surprise.sap", "7.8K",
            "Surprise_2.sap", "9.9K",
            "Third.sap", "9.6K"
        )
        test(this, entries)
    }
}
