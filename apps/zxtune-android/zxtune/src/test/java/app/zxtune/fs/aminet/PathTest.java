package app.zxtune.fs.aminet;

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
// dir/file.lha
@RunWith(RobolectricTestRunner.class)
public class PathTest {

  @Test
  public void testEmpty() {
    final Path path = Path.create();
    verifyRoot(path);
    verifyDir(path.getChild("dir/"));
    verifyFile(path.getChild("dir/file.lha"));
  }

  @Test
  public void testRoot() {
    final Path path = Path.parse(Uri.parse("aminet:"));
    verifyRoot(path);
    assertSame(path, Path.parse(Uri.parse("aminet:/")));
  }

  @Test
  public void testDir() {
    final Path path = Path.parse(Uri.parse("aminet:/dir/"));
    verifyDir(path);
    verifyRoot(path.getParent());
    verifyFile(path.getChild("file.lha"));
    verifyFile(path.getChild("/dir/file.lha"));
  }

  @Test
  public void testFile() {
    final Path path = Path.parse(Uri.parse("aminet:/dir/file.lha"));
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
    assertEquals("getRemoteUris[0]", BuildConfig.CDN_ROOT + "/download/aminet/mods/", uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://aminet.net/mods/", uris[1].toString());
    assertEquals("getLocalId", "", path.getLocalId());
    assertEquals("getUri", "aminet:", path.getUri().toString());
    assertEquals("getName", "", path.getName());
    assertNull("getParent", path.getParent());
    assertTrue("isEmpty", path.isEmpty());
    assertFalse("isFile", path.isFile());
  }

  private static void verifyDir(app.zxtune.fs.httpdir.Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 2, uris.length);
    assertEquals("getRemoteUris[0]", BuildConfig.CDN_ROOT + "/download/aminet/mods/dir/",
        uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://aminet.net/mods/dir/", uris[1].toString());
    assertEquals("getLocalId", "dir", path.getLocalId());
    assertEquals("getUri", "aminet:/dir/", path.getUri().toString());
    assertEquals("getName", "dir", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertFalse("isFile", path.isFile());
  }

  private static void verifyFile(app.zxtune.fs.httpdir.Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 2, uris.length);
    assertEquals("getRemoteUris[0]", BuildConfig.CDN_ROOT + "/download/aminet/mods/dir/file.lha", uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://aminet.net/mods/dir/file.lha", uris[1].toString());
    assertEquals("getLocalId", "dir/file.lha", path.getLocalId());
    assertEquals("getUri", "aminet:/dir/file.lha", path.getUri().toString());
    assertEquals("getName", "file.lha", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertTrue("isFile", path.isFile());
  }
}
