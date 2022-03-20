package app.zxtune.fs.hvsc

import android.net.Uri
import app.zxtune.BuildConfig
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

// dir/
// dir/file.sid
@RunWith(RobolectricTestRunner::class)
class PathTest {

    @Test
    fun `test empty`() = with(Path.create()) {
        verifyRoot(this)
        verifyDir(getChild("dir/"))
    }

    @Test
    fun `test root`() = with(Path.parse(Uri.parse("hvsc:"))!!) {
        verifyRoot(this)
        assertSame(this, Path.parse(Uri.parse("hvsc:/")))
    }

    @Test
    fun `test dir`() = with(Path.parse(Uri.parse("hvsc:/dir/"))!!) {
        verifyDir(this)
        verifyRoot(getParent()!!)
        verifyFile(getChild("file.sid"))
    }

    @Test
    fun `test file`() = with(Path.parse(Uri.parse("hvsc:/dir/file.sid"))!!) {
        verifyFile(this)
        verifyDir(getParent()!!)
    }

    @Test
    fun `test foreign`() = with(Path.parse(Uri.parse("foreign:/uri/test"))) {
        assertNull(this)
    }

    @Test
    fun `test compatibility`() =
        with(Path.parse(Uri.parse("hvsc:/C64Music/DEMOS/0-9/1_45_Tune.sid"))!!) {
            with(getRemoteUris()) {
                assertEquals("getRemoteUris.length", 3, size)
                assertEquals(
                    "getRemoteUris[0]",
                    "${BuildConfig.CDN_ROOT}/browse/hvsc/DEMOS/0-9/1_45_Tune.sid",
                    get(0).toString()
                )
                assertEquals(
                    "getRemoteUris[1]",
                    "https://www.prg.dtu.dk/HVSC/C64Music/DEMOS/0-9/1_45_Tune.sid",
                    get(1).toString()
                )
                assertEquals(
                    "getRemoteUris[2]",
                    "http://www.c64.org/HVSC/DEMOS/0-9/1_45_Tune.sid",
                    get(2).toString()
                )
            }
            assertEquals("getLocalId", "DEMOS/0-9/1_45_Tune.sid", getLocalId())
            assertEquals("getUri", "hvsc:/DEMOS/0-9/1_45_Tune.sid", getUri().toString())
            assertEquals("getName", "1_45_Tune.sid", getName())
            assertFalse("isEmpty", isEmpty())
            assertTrue("isFile", isFile())
        }
}

private fun verifyRoot(path: app.zxtune.fs.httpdir.Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUris.length", 3, size)
        assertEquals("getRemoteUris[0]", "${BuildConfig.CDN_ROOT}/browse/hvsc/", get(0).toString())
        assertEquals("getRemoteUris[1]", "https://www.prg.dtu.dk/HVSC/C64Music/", get(1).toString())
        assertEquals("getRemoteUris[2]", "http://www.c64.org/HVSC/", get(2).toString())
    }
    assertEquals("getLocalId", "", getLocalId())
    assertEquals("getUri", "hvsc:", getUri().toString())
    assertEquals("getName", "", getName())
    assertNull("getParent", getParent())
    assertTrue("isEmpty", isEmpty())
    assertFalse("isFile", isFile())
}

private fun verifyDir(path: app.zxtune.fs.httpdir.Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUris.length", 3, size)
        assertEquals(
            "getRemoteUris[0]",
            "${BuildConfig.CDN_ROOT}/browse/hvsc/dir/",
            get(0).toString()
        )
        assertEquals(
            "getRemoteUris[1]",
            "https://www.prg.dtu.dk/HVSC/C64Music/dir/",
            get(1).toString()
        )
        assertEquals("getRemoteUris[2]", "http://www.c64.org/HVSC/dir/", get(2).toString())
    }
    assertEquals("getLocalId", "dir", getLocalId())
    assertEquals("getUri", "hvsc:/dir/", getUri().toString())
    assertEquals("getName", "dir", getName())
    assertFalse("isEmpty", isEmpty())
    assertFalse("isFile", isFile())
}

private fun verifyFile(path: app.zxtune.fs.httpdir.Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUris.length", 3, size)
        assertEquals(
            "getRemoteUris[0]",
            "${BuildConfig.CDN_ROOT}/browse/hvsc/dir/file.sid",
            get(0).toString()
        )
        assertEquals(
            "getRemoteUris[1]",
            "https://www.prg.dtu.dk/HVSC/C64Music/dir/file.sid",
            get(1).toString()
        )
        assertEquals("getRemoteUris[2]", "http://www.c64.org/HVSC/dir/file.sid", get(2).toString())
    }
    assertEquals("getLocalId", "dir/file.sid", getLocalId())
    assertEquals("getUri", "hvsc:/dir/file.sid", getUri().toString())
    assertEquals("getName", "file.sid", getName())
    assertFalse("isEmpty", isEmpty())
    assertTrue("isFile", isFile())
}
