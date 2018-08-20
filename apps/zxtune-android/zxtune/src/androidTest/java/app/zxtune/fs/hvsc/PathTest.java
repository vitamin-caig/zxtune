package app.zxtune.fs.hvsc;

import android.net.Uri;
import android.support.test.runner.AndroidJUnit4;

import static org.junit.Assert.*;

import org.junit.Test;
import org.junit.runner.RunWith;

// dir/file.sid
@RunWith(AndroidJUnit4.class)
public class PathTest {

  @Test
  public void testEmpty() {
    final Path path = Path.create();
    verifyEmpty(path);
    verifyDir(path.getChild("dir"));
  }

  @Test
  public void testDir() {
    final Path path = Path.parse(Uri.parse("hvsc:/dir"));
    verifyDir(path);
    verifyEmpty(path.getParent());
    verifyFile(path.getChild("file.sid"));
  }

  @Test
  public void testFile() {
    final Path path = Path.parse(Uri.parse("hvsc:/dir/file.sid"));
    verifyFile(path);
    verifyDir(path.getParent());
  }

  @Test
  public void testForeign() {
    final Path path = Path.parse(Uri.parse("foreign:/uri/test"));
    assertEquals(null, path);
  }

  @Test
  public void testCompat() {
    final Path path = Path.parse(Uri.parse("hvsc:/C64Music/DEMOS/0-9/1_45_Tune.sid"));
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 3, uris.length);
    assertEquals("getRemoteUris[0]", "https://storage.zxtune.ru/browse/hvsc/DEMOS/0-9/1_45_Tune.sid", uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://www.prg.dtu.dk/HVSC/C64Music/DEMOS/0-9/1_45_Tune.sid", uris[1].toString());
    assertEquals("getRemoteUris[2]", "http://www.c64.org/HVSC/DEMOS/0-9/1_45_Tune.sid", uris[2].toString());
    assertEquals("getLocalId", "DEMOS/0-9/1_45_Tune.sid", path.getLocalId());
    assertEquals("getUri", "hvsc:/DEMOS/0-9/1_45_Tune.sid", path.getUri().toString());
    assertEquals("getName", "1_45_Tune.sid", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertTrue("isFile", path.isFile());
  }

  private static void verifyEmpty(Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 3, uris.length);
    assertEquals("getRemoteUris[0]", "https://storage.zxtune.ru/browse/hvsc/", uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://www.prg.dtu.dk/HVSC/C64Music/", uris[1].toString());
    assertEquals("getRemoteUris[2]", "http://www.c64.org/HVSC/", uris[2].toString());
    assertEquals("getLocalId", "", path.getLocalId());
    assertEquals("getUri", "hvsc:", path.getUri().toString());
    assertEquals("getName", null, path.getName());
    assertEquals("getParent", null, path.getParent());
    assertTrue("isEmpty", path.isEmpty());
    assertFalse("isFile", path.isFile());
  }

  private static void verifyDir(Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 3, uris.length);
    assertEquals("getRemoteUris[0]", "https://storage.zxtune.ru/browse/hvsc/dir/", uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://www.prg.dtu.dk/HVSC/C64Music/dir/", uris[1].toString());
    assertEquals("getRemoteUris[2]", "http://www.c64.org/HVSC/dir/", uris[2].toString());
    assertEquals("getLocalId", "dir", path.getLocalId());
    assertEquals("getUri", "hvsc:/dir", path.getUri().toString());
    assertEquals("getName", "dir", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertFalse("isFile", path.isFile());
  }

  private static void verifyFile(Path path) {
    final Uri[] uris = path.getRemoteUris();
    assertEquals("getRemoteUris.length", 3, uris.length);
    assertEquals("getRemoteUris[0]", "https://storage.zxtune.ru/browse/hvsc/dir/file.sid", uris[0].toString());
    assertEquals("getRemoteUris[1]", "http://www.prg.dtu.dk/HVSC/C64Music/dir/file.sid", uris[1].toString());
    assertEquals("getRemoteUris[2]", "http://www.c64.org/HVSC/dir/file.sid", uris[2].toString());
    assertEquals("getLocalId", "dir/file.sid", path.getLocalId());
    assertEquals("getUri", "hvsc:/dir/file.sid", path.getUri().toString());
    assertEquals("getName", "file.sid", path.getName());
    assertFalse("isEmpty", path.isEmpty());
    assertTrue("isFile", path.isFile());
  }
}
