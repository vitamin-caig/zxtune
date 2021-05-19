package app.zxtune.fs.scene

import android.net.Uri
import app.zxtune.BuildConfig
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

// dir/
// dir/file.mp3
@RunWith(RobolectricTestRunner::class)
class PathTest {

    @Test
    fun `test empty`() = with(Path.create()) {
        verifyRoot(this)
        verifyDir(getChild("dir/"))
    }

    @Test
    fun `test root`() = with(Path.parse(Uri.parse("scene:"))!!) {
        verifyRoot(this)
        assertSame(this, Path.parse(Uri.parse("scene:/")))
    }

    @Test
    fun `test dir`() = with(Path.parse(Uri.parse("scene:/dir/"))!!) {
        verifyDir(this)
        verifyRoot(getParent()!!)
        verifyFile(getChild("file.mp3"))
    }

    @Test
    fun `test file`() = with(Path.parse(Uri.parse("scene:/dir/file.mp3"))!!) {
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
            "getRemoteUris[0]", "${BuildConfig.CDN_ROOT}/browse/scene/music/", get(0).toString()
        )
        assertEquals("getRemoteUris[1]", "https://archive.scene.org/pub/music/", get(1).toString())
    }
    assertEquals("getLocalId", "", getLocalId())
    assertEquals("getUri", "scene:", getUri().toString())
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
            "${BuildConfig.CDN_ROOT}/browse/scene/music/dir/",
            get(0).toString()
        )
        assertEquals(
            "getRemoteUris[1]", "https://archive.scene.org/pub/music/dir/", get(1).toString()
        )
    }
    assertEquals("getLocalId", "dir", getLocalId())
    assertEquals("getUri", "scene:/dir/", getUri().toString())
    assertEquals("getName", "dir", getName())
    assertFalse("isEmpty", isEmpty())
    assertFalse("isFile", isFile())
}

private fun verifyFile(path: app.zxtune.fs.httpdir.Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUris.length", 2, size)
        assertEquals(
            "getRemoteUris[0]",
            "${BuildConfig.CDN_ROOT}/browse/scene/music/dir/file.mp3",
            get(0).toString()
        )
        assertEquals(
            "getRemoteUris[1]",
            "https://archive.scene.org/pub/music/dir/file.mp3",
            get(1).toString()
        )
    }
    assertEquals("getLocalId", "dir/file.mp3", getLocalId())
    assertEquals("getUri", "scene:/dir/file.mp3", getUri().toString())
    assertEquals("getName", "file.mp3", getName())
    assertFalse("isEmpty", isEmpty())
    assertTrue("isFile", isFile())
}
