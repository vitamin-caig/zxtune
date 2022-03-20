package app.zxtune.fs.aminet

import android.net.Uri
import app.zxtune.BuildConfig
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

// dir/
// dir/file.lha
@RunWith(RobolectricTestRunner::class)
class PathTest {

    @Test
    fun `test empty`() = with(Path.create()) {
        verifyRoot(this)
        verifyDir(getChild("dir/"))
        verifyFile(getChild("dir/file.lha"))
    }

    @Test
    fun `test root`() = with(Path.parse(Uri.parse("aminet:"))!!) {
        verifyRoot(this)
        assertSame(this, Path.parse(Uri.parse("aminet:/")))
    }

    @Test
    fun `test dir`() = with(Path.parse(Uri.parse("aminet:/dir/"))!!) {
        verifyDir(this)
        verifyRoot(getParent()!!)
        verifyFile(getChild("file.lha"))
        verifyFile(getChild("/dir/file.lha"))
    }

    @Test
    fun `test file`() = with(Path.parse(Uri.parse("aminet:/dir/file.lha"))!!) {
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
        assertEquals(
            "getRemoteUris[0]",
            "${BuildConfig.CDN_ROOT}/download/aminet/mods/",
            get(0).toString()
        )
        assertEquals("getRemoteUris[1]", "http://aminet.net/mods/", get(1).toString())
    }
    assertEquals("getLocalId", "", getLocalId())
    assertEquals("getUri", "aminet:", getUri().toString())
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
            "${BuildConfig.CDN_ROOT}/download/aminet/mods/dir/",
            get(0).toString()
        )
        assertEquals("getRemoteUris[1]", "http://aminet.net/mods/dir/", get(1).toString())
    }
    assertEquals("getLocalId", "dir", getLocalId())
    assertEquals("getUri", "aminet:/dir/", getUri().toString())
    assertEquals("getName", "dir", getName())
    assertFalse("isEmpty", isEmpty())
    assertFalse("isFile", isFile())
}

private fun verifyFile(path: app.zxtune.fs.httpdir.Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUris.length", 2, size)
        assertEquals(
            "getRemoteUris[0]",
            "${BuildConfig.CDN_ROOT}/download/aminet/mods/dir/file.lha",
            get(0).toString()
        )
        assertEquals("getRemoteUris[1]", "http://aminet.net/mods/dir/file.lha", get(1).toString())
    }
    assertEquals("getLocalId", "dir/file.lha", getLocalId())
    assertEquals("getUri", "aminet:/dir/file.lha", getUri().toString())
    assertEquals("getName", "file.lha", getName())
    assertFalse("isEmpty", isEmpty())
    assertTrue("isFile", isFile())
}
