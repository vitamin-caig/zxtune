package app.zxtune.fs.asma

import android.net.Uri
import app.zxtune.BuildConfig
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

// dir/
// dir/file.sap
@RunWith(RobolectricTestRunner::class)
class PathTest {

    @Test
    fun `test empty`() = with(Path.create()) {
        verifyRoot(this)
        verifyDir(getChild("dir/"))
    }

    @Test
    fun `test root`() = with(Path.parse(Uri.parse("asma:"))!!) {
        verifyRoot(this)
        assertSame(this, Path.parse(Uri.parse("asma:/")))
    }

    @Test
    fun `test dir`() = with(Path.parse(Uri.parse("asma:/dir/"))!!) {
        verifyDir(this)
        verifyRoot(getParent()!!)
        verifyFile(getChild("file.sap"))
    }

    @Test
    fun `test file`() = with(Path.parse(Uri.parse("asma:/dir/file.sap"))!!) {
        verifyFile(this)
        verifyDir(getParent()!!)
    }

    @Test
    fun `test foreign`() = with(Path.parse(Uri.parse("foreign:/uri/test"))) {
        assertNull(this)
    }
}

private fun verifyRoot(path: app.zxtune.fs.httpdir.Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUris.length", 2, size)
        assertEquals("getRemoteUris[0]", "${BuildConfig.CDN_ROOT}/browse/asma/", get(0).toString())
        assertEquals("getRemoteUris[1]", "http://asma.atari.org/asma/", get(1).toString())
    }
    assertEquals("getLocalId", "", getLocalId())
    assertEquals("getUri", "asma:", getUri().toString())
    assertEquals("getName", "", getName())
    assertNull("getParent", getParent())
    assertTrue("isEmpty", isEmpty())
    assertFalse("isFile", isFile())
}

private fun verifyDir(path: app.zxtune.fs.httpdir.Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUris.length", 2, size)
        assertEquals(
            "getRemoteUris[0]",
            "${BuildConfig.CDN_ROOT}/browse/asma/dir/",
            get(0).toString()
        )
        assertEquals("getRemoteUris[1]", "http://asma.atari.org/asma/dir/", get(1).toString())
    }
    assertEquals("getLocalId", "dir", getLocalId())
    assertEquals("getUri", "asma:/dir/", getUri().toString())
    assertEquals("getName", "dir", getName())
    assertFalse("isEmpty", isEmpty())
    assertFalse("isFile", isFile())
}

private fun verifyFile(path: app.zxtune.fs.httpdir.Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUris.length", 2, size)
        assertEquals(
            "getRemoteUris[0]",
            "${BuildConfig.CDN_ROOT}/browse/asma/dir/file.sap",
            get(0).toString()
        )
        assertEquals(
            "getRemoteUris[1]", "http://asma.atari.org/asma/dir/file.sap", get(1).toString()
        )
    }
    assertEquals("getLocalId", "dir/file.sap", getLocalId())
    assertEquals("getUri", "asma:/dir/file.sap", getUri().toString())
    assertEquals("getName", "file.sap", getName())
    assertFalse("isEmpty", isEmpty())
    assertTrue("isFile", isFile())
}
