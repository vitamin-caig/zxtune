package app.zxtune.fs.aygor;

import android.net.Uri;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

// dir/
// dir/file.ay
@RunWith(RobolectricTestRunner.class)
public class PathTest {

  @Test
  public void testEmpty() {
    final Path path = Path.create();
    verifyRoot(path);
    verifyDir(path.getChild("dir/"));
  }

  @Test
  public void testRoot() {
    final Path path = Path.parse(Uri.parse("aygor:"));
    verifyRoot(path);
    assertSame(path, Path.parse(Uri.parse("aygor:/")));
  }

  @Test
  public void testDir() {
    final Path path = Path.parse(Uri.parse("aygor:/dir/"));
    verifyDir(path);
    verifyRoot(path.getParent());
    verifyFile(path.getChild("file.ay"));
  }

  @Test
  public void testFile() {
    final Path path = Path.parse(Uri.parse("aygor:/dir/file.ay"));
    verifyFile(path);
    verifyDir(path.getParent());
  }

  @Test
  public void testForeign() {
    final Path path = Path.parse(Uri.parse("foreign:/uri/test"));
    assertNull(path);
  }

  @Test
  public void testEscaping() {
    final Path path = Path.parse(Uri.parse("aygor:/games/Commando[CPC].ay"));
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 1, uris.length);
    assertEquals("getRemoteUris[0]", "http://abrimaal.pro-e.pl/ayon/games/Commando%5BCPC%5D.ay", uris[0].toString());
    assertEquals("getLocalId", "games/Commando[CPC].ay", path.getLocalId());
    assertEquals("getUri", "aygor:/games/Commando%5BCPC%5D.ay", path.getUri().toString());
    assertEquals("getName", "Commando[CPC].ay", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertTrue("isFile", path.isFile());
  }

  private static void verifyRoot(app.zxtune.fs.httpdir.Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 1, uris.length);
    assertEquals("getRemoteUris[0]", "http://abrimaal.pro-e.pl/ayon/", uris[0].toString());
    assertEquals("getLocalId", "", path.getLocalId());
    assertEquals("getUri", "aygor:", path.getUri().toString());
    assertEquals("getName", "", path.getName());
    assertNull("getParent", path.getParent());
    assertTrue("isEmpty", path.isEmpty());
    assertFalse("isFile", path.isFile());
  }

  private static void verifyDir(app.zxtune.fs.httpdir.Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 1, uris.length);
    assertEquals("getRemoteUris[0]", "http://abrimaal.pro-e.pl/ayon/dir/", uris[0].toString());
    assertEquals("getLocalId", "dir", path.getLocalId());
    assertEquals("getUri", "aygor:/dir/", path.getUri().toString());
    assertEquals("getName", "dir", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertFalse("isFile", path.isFile());
  }

  private static void verifyFile(app.zxtune.fs.httpdir.Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 1, uris.length);
    assertEquals("getRemoteUris[0]", "http://abrimaal.pro-e.pl/ayon/dir/file.ay", uris[0].toString());
    assertEquals("getLocalId", "dir/file.ay", path.getLocalId());
    assertEquals("getUri", "aygor:/dir/file.ay", path.getUri().toString());
    assertEquals("getName", "file.ay", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertTrue("isFile", path.isFile());
  }
}
