package app.zxtune.fs.aminet

import android.net.Uri
import app.zxtune.fs.httpdir.RemoteCatalogTestBase
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class RemoteCatalogTest : RemoteCatalogTestBase() {

    @Before
    fun setUp() = setUpTest(::RemoteCatalog)

    @Test
    fun `test root`() = with(Path.create()) {
        //hardcoded entries, filtered in Root
        val entries = arrayOf(
            "32bit", "@MP3s by 32bit choonz (2 packages)",
            "xm", "@Multichannel xm mods (413 packages)"
        )
        test(this, entries, Mode.CHECK_MISSED)
    }

    @Test
    fun `test listing`() = with(Path.parse(Uri.parse("aminet:/piano"))!!) {
        val entries = arrayOf(
            "/piano/BananaRag.lha", "33K@Banana Rag. Cool rag tune by HeLMeR",  //last one
            "/piano/wzl_laudamus.lzh", "99K@Protracker module by The Weasel ****+"
        )
        test(this, entries, Mode.CHECK_MISSED)
    }

    @Test
    fun `test search`() {
        val entries = arrayOf( //filename
            "/misc/BeginLife.lha", "116K@Rymix/[Death] mod: 'Beginning of Life'",  //description
            "/neuro/nd-cls03.lha", "105K@The new Beginning (older Module by Neurodancer/1oo%)"
        )
        with(CheckingVisitor(entries, Mode.CHECK_MISSED)) {
            (catalog as RemoteCatalog).find("begin", this)
            check()
        }
    }
}
