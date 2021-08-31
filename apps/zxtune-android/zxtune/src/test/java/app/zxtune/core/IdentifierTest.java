package app.zxtune.core;

import static org.junit.Assert.assertEquals;

import android.net.Uri;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;

@RunWith(RobolectricTestRunner.class)
public class IdentifierTest {
  @Test
  public void parseEmpty() {
    Identifier id = Identifier.parse(null);

    assertEquals("", id.toString());
    assertEquals(Uri.EMPTY, id.getFullLocation());
    assertEquals(Uri.EMPTY, id.getDataLocation());
    assertEquals("", id.getSubPath());
    assertEquals("", id.getDisplayFilename());

    verifyEquals(id, Identifier.parse(""));
    verifyEquals(id, new Identifier(Uri.EMPTY));
    verifyEquals(id, new Identifier(Uri.EMPTY, null));
    verifyEquals(id, new Identifier(Uri.EMPTY, ""));
    verifyEquals(id, new Identifier(Uri.EMPTY, "ignored"));
  }

  private static void verifyEquals(Identifier lh, Identifier rh) {
    assertEquals(lh, rh);
    assertEquals(lh.toString(), rh.toString());
  }

  @Test
  public void parseFile() {
    String path = "dir/file";
    Identifier id = Identifier.parse(path);

    assertEquals(path, id.toString());
    assertEquals(path, id.getFullLocation().toString());
    assertEquals(path, id.getDataLocation().toString());
    assertEquals("", id.getSubPath());
    assertEquals("file", id.getDisplayFilename());

    verifyEquals(id, Identifier.parse(path + "#"));
    verifyEquals(id, new Identifier(Uri.parse(path)));
    verifyEquals(id, new Identifier(Uri.parse(path), null));
    verifyEquals(id, new Identifier(Uri.parse(path), ""));
  }

  @Test
  public void parseFull() {
    String path = "http://host/path?query";
    String subPath = "nested/sub path";
    String subPathEncoded = Uri.encode(subPath);

    Identifier id = Identifier.parse(path + '#' + subPathEncoded);

    assertEquals(path + '#' + subPathEncoded, id.toString());
    assertEquals(path + '#' + subPathEncoded, id.getFullLocation().toString());
    assertEquals(path, id.getDataLocation().toString());
    assertEquals("nested/sub path", id.getSubPath());
    assertEquals("path > sub path", id.getDisplayFilename());

    verifyEquals(id, new Identifier(Uri.parse(path), subPath));
  }
}
