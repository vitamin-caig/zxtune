package app.zxtune.fs.scene;

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
// dir/file.mp3
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
    final Path path = Path.parse(Uri.parse("scene:"));
    verifyRoot(path);
    assertSame(path, Path.parse(Uri.parse("scene:/")));
  }

  @Test
  public void testDir() {
    final Path path = Path.parse(Uri.parse("scene:/dir/"));
    verifyDir(path);
    verifyRoot(path.getParent());
    verifyFile(path.getChild("file.mp3"));
  }

  @Test
  public void testFile() {
    final Path path = Path.parse(Uri.parse("scene:/dir/file.mp3"));
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
    assertEquals("getRemoteUris[0]", BuildConfig.CDN_ROOT + "/browse/scene/music/",
        uris[0].toString());
    assertEquals("getRemoteUris[1]", "https://archive.scene.org/pub/music/", uris[1].toString());
    assertEquals("getLocalId", "", path.getLocalId());
    assertEquals("getUri", "scene:", path.getUri().toString());
    assertEquals("getName", "", path.getName());
    assertNull("getParent", path.getParent());
    assertTrue("isEmpty", path.isEmpty());
    assertFalse("isFile", path.isFile());
  }

  private static void verifyDir(app.zxtune.fs.httpdir.Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 2, uris.length);
    assertEquals("getRemoteUris[0]", BuildConfig.CDN_ROOT + "/browse/scene/music/dir/", uris[0].toString());
    assertEquals("getRemoteUris[1]", "https://archive.scene.org/pub/music/dir/", uris[1].toString());
    assertEquals("getLocalId", "dir", path.getLocalId());
    assertEquals("getUri", "scene:/dir/", path.getUri().toString());
    assertEquals("getName", "dir", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertFalse("isFile", path.isFile());
  }

  private static void verifyFile(app.zxtune.fs.httpdir.Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 2, uris.length);
    assertEquals("getRemoteUris[0]", BuildConfig.CDN_ROOT + "/browse/scene/music/dir/file.mp3",
        uris[0].toString());
    assertEquals("getRemoteUris[1]", "https://archive.scene.org/pub/music/dir/file.mp3",
        uris[1].toString());
    assertEquals("getLocalId", "dir/file.mp3", path.getLocalId());
    assertEquals("getUri", "scene:/dir/file.mp3", path.getUri().toString());
    assertEquals("getName", "file.mp3", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertTrue("isFile", path.isFile());
  }
}
