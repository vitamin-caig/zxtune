package app.zxtune.fs.aminet

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
    fun setUp() = setUpTest(::RemoteCatalog)

    @Test
    fun `test root`() = with(Path.create()) {
        //hardcoded entries, filtered in Root
        val entries = arrayOf(
            "32bit", "@MP3s by 32bit choonz (6 packages)",
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
            "/misc/BeginLife.lha", "116K@Rymix/[Death] mod: \\'Beginning of Life\\'",  //description
            "/wmr/newbegin.lha", "155K@IMM-Mon: \\\"A New Beginning\\\" by Deeno"
        )
        with(CheckingVisitor(entries, Mode.CHECK_MISSED)) {
            (catalog as RemoteCatalog).find("begin", this)
            check()
        }
    }
}
