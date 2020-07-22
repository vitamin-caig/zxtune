package app.zxtune.fs.asma;

import android.net.Uri;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

import app.zxtune.BuildConfig;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;

// dir/
// dir/file.sap
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
    final Path path = Path.parse(Uri.parse("asma:"));
    verifyRoot(path);
    assertSame(path, Path.parse(Uri.parse("asma:/")));
  }

  @Test
  public void testDir() {
    final Path path = Path.parse(Uri.parse("asma:/dir/"));
    verifyDir(path);
    verifyRoot(path.getParent());
    verifyFile(path.getChild("file.sap"));
  }

  @Test
  public void testFile() {
    final Path path = Path.parse(Uri.parse("asma:/dir/file.sap"));
    verifyFile(path);
    verifyDir(path.getParent());
  }

  @Test
  public void testForeign() {
    final Path path = Path.parse(Uri.parse("foreign:/uri/test"));
    assertNull(path);
  }

  private static void verifyRoot(app.zxtune.fs.httpdir.Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 2, uris.length);
    assertEquals("getRemoteUris[0]", BuildConfig.CDN_ROOT + "/browse/asma/", uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://asma.atari.org/asma/", uris[1].toString());
    assertEquals("getLocalId", "", path.getLocalId());
    assertEquals("getUri", "asma:", path.getUri().toString());
    assertEquals("getName", "", path.getName());
    assertNull("getParent", path.getParent());
    assertTrue("isEmpty", path.isEmpty());
    assertFalse("isFile", path.isFile());
  }

  private static void verifyDir(app.zxtune.fs.httpdir.Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 2, uris.length);
    assertEquals("getRemoteUris[0]", BuildConfig.CDN_ROOT + "/browse/asma/dir/", uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://asma.atari.org/asma/dir/", uris[1].toString());
    assertEquals("getLocalId", "dir", path.getLocalId());
    assertEquals("getUri", "asma:/dir/", path.getUri().toString());
    assertEquals("getName", "dir", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertFalse("isFile", path.isFile());
  }

  private static void verifyFile(app.zxtune.fs.httpdir.Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 2, uris.length);
    assertEquals("getRemoteUris[0]", BuildConfig.CDN_ROOT + "/browse/asma/dir/file.sap", uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://asma.atari.org/asma/dir/file.sap", uris[1].toString());
    assertEquals("getLocalId", "dir/file.sap", path.getLocalId());
    assertEquals("getUri", "asma:/dir/file.sap", path.getUri().toString());
    assertEquals("getName", "file.sap", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertTrue("isFile", path.isFile());
  }
}
