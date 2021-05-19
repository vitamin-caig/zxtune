package app.zxtune.fs.joshw

import android.net.Uri
import app.zxtune.BuildConfig
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

// root/dir/file.7z
@RunWith(RobolectricTestRunner::class)
class PathTest {

    @Test
    fun `test empty`() = with(Path.create()) {
        verifyRoot(this)
        verifyCatalog(getChild("root") as Path)
    }

    @Test
    fun `test root`() = with(Path.parse(Uri.parse("joshw:"))!!) {
        verifyRoot(this)
        assertSame(this, Path.parse(Uri.parse("joshw:/")))
    }

    @Test
    fun `test catalog`() = with(Path.parse(Uri.parse("joshw:/root"))!!) {
        // Root is dir in spite of slash absence
        verifyCatalog(this)
        verifyRoot(getParent() as Path)
        verifyDir(getChild("dir/") as Path)
    }

    @Test
    fun `test dir`() = with(Path.parse(Uri.parse("joshw:/root/dir/"))!!) {
        verifyDir(this)
        verifyCatalog(getParent() as Path)
        verifyFile(getChild("file.7z") as Path)
    }

    @Test
    fun `test file`() = with(Path.parse(Uri.parse("joshw:/root/dir/file.7z"))!!) {
        verifyFile(this)
        verifyDir(getParent() as Path)
    }

    @Test
    fun `test foreign`() = with(Path.parse(Uri.parse("foreign:/uri/test"))) {
        assertNull(this)
    }

    @Test
    fun `test escaping`() =
        with(Path.parse(Uri.parse("joshw:/nsf/n/North%20& South (1990-09-21)(Kemco).7z"))!!) {
            with(getRemoteUris()) {
                assertEquals("getRemoteUri.length", 2, size)
                assertEquals(
                    "getRemoteUris[0]",
                    "${BuildConfig.CDN_ROOT}/browse/joshw/nsf/n/North%20%26%20South%20(1990-09-21)(Kemco).7z",
                    get(0).toString()
                )
                assertEquals(
                    "getRemoteUris[1]",
                    "http://nsf.joshw.info/n/North%20%26%20South%20(1990-09-21)(Kemco).7z",
                    get(1).toString()
                )
            }
            assertEquals("getLocalId", "nsf/n/North & South (1990-09-21)(Kemco).7z", getLocalId())
            assertEquals(
                "getUri",
                "joshw:/nsf/n/North%20%26%20South%20(1990-09-21)(Kemco).7z",
                getUri().toString()
            )
            assertEquals("getName", "North & South (1990-09-21)(Kemco).7z", getName())
            assertFalse("isEmpty", isEmpty())
            assertTrue("isFile", isFile())
            assertFalse("isCatalogue", isCatalogue())
        }
}

private fun verifyRoot(path: Path) = with(path) {
    assertEquals("getUri", "joshw:", getUri().toString())
    assertEquals("", getName())
    assertNull("getParent", getParent())
    assertTrue("isEmpty", isEmpty())
    assertFalse("isFile", isFile())
    assertFalse("isCatalogue", isCatalogue())
}

private fun verifyCatalog(path: Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUri.length", 2, size)
        assertEquals(
            "getRemoteUris[0]",
            "${BuildConfig.CDN_ROOT}/browse/joshw/root/",
            get(0).toString()
        )
        assertEquals("getRemoteUris[1]", "http://root.joshw.info", get(1).toString())
    }
    assertEquals("getLocalId", "root", getLocalId())
    assertEquals("getUri", "joshw:/root/", getUri().toString())
    assertEquals("getName", "root", getName())
    assertFalse("isEmpty", isEmpty())
    assertFalse("isFile", isFile())
    assertTrue("isCatalogue", isCatalogue())
}

private fun verifyDir(path: Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUri.length", 2, size)
        assertEquals(
            "getRemoteUris[0]",
            "${BuildConfig.CDN_ROOT}/browse/joshw/root/dir/",
            get(0).toString()
        )
        assertEquals("getRemoteUris[1]", "http://root.joshw.info/dir/", get(1).toString())
    }
    assertEquals("getLocalId", "root/dir", getLocalId())
    assertEquals("getUri", "joshw:/root/dir/", getUri().toString())
    assertEquals("getName", "dir", getName())
    assertFalse("isEmpty", isEmpty())
    assertFalse("isFile", isFile())
    assertFalse("isCatalogue", isCatalogue())
}

private fun verifyFile(path: Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUri.length", 2, size)
        assertEquals(
            "getRemoteUris[0]",
            "${BuildConfig.CDN_ROOT}/browse/joshw/root/dir/file.7z",
            get(0).toString()
        )
        assertEquals("getRemoteUris[1]", "http://root.joshw.info/dir/file.7z", get(1).toString())
    }
    assertEquals("getLocalId", "root/dir/file.7z", getLocalId())
    assertEquals("getUri", "joshw:/root/dir/file.7z", getUri().toString())
    assertEquals("getName", "file.7z", getName())
    assertFalse("isEmpty", isEmpty())
    assertTrue("isFile", isFile())
    assertFalse("isCatalogue", isCatalogue())
}
