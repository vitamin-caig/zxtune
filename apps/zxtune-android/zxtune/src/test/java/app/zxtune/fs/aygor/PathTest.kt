package app.zxtune.fs.aygor

import android.net.Uri
import org.junit.Assert.*

import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

// dir/
// dir/file.ay
@RunWith(RobolectricTestRunner::class)
class PathTest {

    @Test
    fun `test empty`() = with(Path.create()) {
        verifyRoot(this)
        verifyDir(getChild("dir/"))
    }

    @Test
    fun `test root`() = with(Path.parse(Uri.parse("aygor:"))!!) {
        verifyRoot(this)
        assertSame(this, Path.parse(Uri.parse("aygor:/")))
    }

    @Test
    fun `test dir`() = with(Path.parse(Uri.parse("aygor:/dir/"))!!) {
        verifyDir(this)
        verifyRoot(getParent()!!)
        verifyFile(getChild("file.ay"))
    }

    @Test
    fun `test file`() = with(Path.parse(Uri.parse("aygor:/dir/file.ay"))!!) {
        verifyFile(this)
        verifyDir(getParent()!!)
    }

    @Test
    fun `test foreign`() = with(Path.parse(Uri.parse("foreign:/uri/test"))) {
        assertNull(this)
    }

    @Test
    fun `test escaping`() = with(Path.parse(Uri.parse("aygor:/games/Commando[CPC].ay"))!!) {
        with(getRemoteUris()) {
            assertEquals("getRemoteUris.length", 1, size)
            assertEquals(
                "getRemoteUris[0]",
                "http://abrimaal.pro-e.pl/ayon/games/Commando%5BCPC%5D.ay",
                get(0).toString()
            )
        }
        assertEquals("getLocalId", "games/Commando[CPC].ay", getLocalId())
        assertEquals("getUri", "aygor:/games/Commando%5BCPC%5D.ay", getUri().toString())
        assertEquals("getName", "Commando[CPC].ay", getName())
        assertFalse("isEmpty", isEmpty())
        assertTrue("isFile", isFile())
    }
}

private fun verifyRoot(path: app.zxtune.fs.httpdir.Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUris.length", 1, size)
        assertEquals("getRemoteUris[0]", "http://abrimaal.pro-e.pl/ayon/", get(0).toString())
    }
    assertEquals("getLocalId", "", getLocalId())
    assertEquals("getUri", "aygor:", getUri().toString())
    assertEquals("getName", "", getName())
    assertNull("getParent", getParent())
    assertTrue("isEmpty", isEmpty())
    assertFalse("isFile", isFile())
}

private fun verifyDir(path: app.zxtune.fs.httpdir.Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUris.length", 1, size)
        assertEquals("getRemoteUris[0]", "http://abrimaal.pro-e.pl/ayon/dir/", get(0).toString())
    }
    assertEquals("getLocalId", "dir", getLocalId())
    assertEquals("getUri", "aygor:/dir/", getUri().toString())
    assertEquals("getName", "dir", getName())
    assertFalse("isEmpty", isEmpty())
    assertFalse("isFile", isFile())
}

private fun verifyFile(path: app.zxtune.fs.httpdir.Path) = with(path) {
    with(getRemoteUris()) {
        assertEquals("getRemoteUris.length", 1, size)
        assertEquals(
            "getRemoteUris[0]",
            "http://abrimaal.pro-e.pl/ayon/dir/file.ay",
            get(0).toString()
        )
    }
    assertEquals("getLocalId", "dir/file.ay", getLocalId())
    assertEquals("getUri", "aygor:/dir/file.ay", getUri().toString())
    assertEquals("getName", "file.ay", getName())
    assertFalse("isEmpty", isEmpty())
    assertTrue("isFile", isFile())
}
