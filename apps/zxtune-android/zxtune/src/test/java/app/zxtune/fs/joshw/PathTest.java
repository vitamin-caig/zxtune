package app.zxtune.fs.joshw;

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

// root/dir/file.7z
@RunWith(RobolectricTestRunner.class)
public class PathTest {

  @Test
  public void testEmpty() {
    final Path path = Path.create();
    verifyRoot(path);
    verifyCatalog((Path) path.getChild("root"));
  }

  @Test
  public void testRoot() {
    final Path path = Path.parse(Uri.parse("joshw:"));
    verifyRoot(path);
    assertSame(path, Path.parse(Uri.parse("joshw:/")));
  }

  @Test
  public void testCatalog() {
    // Root is dir in spite of slash absence
    final Path path = Path.parse(Uri.parse("joshw:/root"));
    verifyCatalog(path);
    verifyRoot((Path) path.getParent());
    verifyDir((Path) path.getChild("dir/"));
  }

  @Test
  public void testDir() {
    final Path path = Path.parse(Uri.parse("joshw:/root/dir/"));
    verifyDir(path);
    verifyCatalog((Path) path.getParent());
    verifyFile((Path) path.getChild("file.7z"));
  }

  @Test
  public void testFile() {
    final Path path = Path.parse(Uri.parse("joshw:/root/dir/file.7z"));
    verifyFile(path);
    verifyDir((Path) path.getParent());
  }

  @Test
  public void testForeign() {
    final Path path = Path.parse(Uri.parse("foreign:/uri/test"));
    assertNull(path);
  }

  @Test
  public void testEscaping() {
    final Path path = Path.parse(Uri.parse("joshw:/nsf/n/North%20& South (1990-09-21)(Kemco).7z"));
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUri.length", 2, uris.length);
    assertEquals("getRemoteUris[0]", BuildConfig.CDN_ROOT + "/browse/joshw/nsf/n/North%20%26%20South%20(1990-09-21)(Kemco).7z", uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://nsf.joshw.info/n/North%20%26%20South%20(1990-09-21)(Kemco).7z", uris[1].toString());
    assertEquals("getLocalId", "nsf/n/North & South (1990-09-21)(Kemco).7z", path.getLocalId());
    assertEquals("getUri", "joshw:/nsf/n/North%20%26%20South%20(1990-09-21)(Kemco).7z", path.getUri().toString());
    assertEquals("getName", "North & South (1990-09-21)(Kemco).7z", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertTrue("isFile", path.isFile());
    assertFalse("isCatalogue", path.isCatalogue());
  }

  private static void verifyRoot(Path path) {
    assertEquals("getUri", "joshw:", path.getUri().toString());
    assertEquals("", path.getName());
    assertNull("getParent", path.getParent());
    assertTrue("isEmpty", path.isEmpty());
    assertFalse("isFile", path.isFile());
    assertFalse("isCatalogue", path.isCatalogue());
  }

  private static void verifyCatalog(Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUri.length", 2, uris.length);
    assertEquals("getRemoteUris[0]", BuildConfig.CDN_ROOT + "/browse/joshw/root/", uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://root.joshw.info", uris[1].toString());
    assertEquals("getLocalId", "root", path.getLocalId());
    assertEquals("getUri", "joshw:/root/", path.getUri().toString());
    assertEquals("getName", "root", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertFalse("isFile", path.isFile());
    assertTrue("isCatalogue", path.isCatalogue());
  }

  private static void verifyDir(Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUri.length", 2, uris.length);
    assertEquals("getRemoteUris[0]", BuildConfig.CDN_ROOT + "/browse/joshw/root/dir/", uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://root.joshw.info/dir/", uris[1].toString());
    assertEquals("getLocalId", "root/dir", path.getLocalId());
    assertEquals("getUri", "joshw:/root/dir/", path.getUri().toString());
    assertEquals("getName", "dir", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertFalse("isFile", path.isFile());
    assertFalse("isCatalogue", path.isCatalogue());
  }

  private static void verifyFile(Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUri.length", 2, uris.length);
    assertEquals("getRemoteUris[0]", BuildConfig.CDN_ROOT + "/browse/joshw/root/dir/file.7z", uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://root.joshw.info/dir/file.7z", uris[1].toString());
    assertEquals("getLocalId", "root/dir/file.7z", path.getLocalId());
    assertEquals("getUri", "joshw:/root/dir/file.7z", path.getUri().toString());
    assertEquals("getName", "file.7z", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertTrue("isFile", path.isFile());
    assertFalse("isCatalogue", path.isCatalogue());
  }
}
