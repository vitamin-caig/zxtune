package app.zxtune.fs;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.content.res.Resources;
import android.net.Uri;

import androidx.annotation.RawRes;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

import app.zxtune.io.Io;
import app.zxtune.io.TransactionalOutputStream;
import app.zxtune.utils.StubProgressCallback;

@RunWith(AndroidJUnit4.class)
public class VfsArchiveTest {

  private Resources resources;
  private File tmpDir;

  private Uri getFile(@RawRes int res, String filename) throws IOException {
    final File result = new File(tmpDir, filename);
    final InputStream in = new BufferedInputStream(resources.openRawResource(res));
    final OutputStream out = new TransactionalOutputStream(result);
    final long size = Io.copy(in, out);
    assertTrue(size > 0);
    out.flush();
    out.close();
    return Uri.fromFile(result);
  }

  @Before
  public void setUp() {
    resources = InstrumentationRegistry.getInstrumentation().getContext().getResources();
    tmpDir = new File(System.getProperty("java.io.tmpdir", "."), String.format("VfsArchiveTest/%d",
        System.currentTimeMillis()));

  }

  @After
  public void tearDown() {
    final File[] files = tmpDir.listFiles();
    if (files != null) {
      for (File f : files) {
        f.delete();
      }
    }
    tmpDir.delete();
  }

  private static void unreachable() {
    fail("Unreachable");
  }

  @Test
  public void testNull() throws IOException {
    try {
      VfsArchive.resolve(null);
      unreachable();
    } catch (NullPointerException e) {
      assertNotNull("Thrown exception", e);
    }
    try {
      VfsArchive.browse(null);
      unreachable();
    } catch (NullPointerException e) {
      assertNotNull("Thrown exception", e);
    }
    try {
      VfsArchive.browseCached(null);
      unreachable();
    } catch (NullPointerException e) {
      assertNotNull("Thrown exception", e);
    }
  }

  @Test
  public void testDir() throws IOException {
    final Uri uri = Uri.parse("file:/proc");
    final VfsDir dir = (VfsDir) VfsArchive.resolve(uri);
    assertNotNull(dir);
    assertFalse(VfsArchive.checkIfArchive(dir));
    assertEquals("proc", dir.getName());
    assertEquals("", dir.getDescription());
    assertEquals(uri, dir.getUri());
  }

  @Test
  public void testFile() throws IOException {
    final Uri uri = Uri.parse("file:/proc/meminfo");
    final VfsFile file = (VfsFile) VfsArchive.resolve(uri);
    assertNotNull(file);
    assertEquals("meminfo", file.getName());
    assertEquals("", file.getDescription());
    assertEquals("0.00B", file.getSize());
    assertEquals(uri, file.getUri());
  }

  @Test
  public void testFileTrack() throws IOException {
    final Uri uri = getFile(app.zxtune.test.R.raw.track, "track");
    final VfsFile file = (VfsFile) VfsArchive.resolve(uri);
    assertNotNull(file);
    assertEquals("track", file.getName());
    assertEquals("", file.getDescription());
    assertEquals("2.8KB", file.getSize());

    final Uri normalizedUri = file.getUri();
    {
      final VfsFile browsed = (VfsFile) VfsArchive.browse(file);
      assertEquals(file, browsed);
    }
    {
      final VfsFile browsedCached = (VfsFile) VfsArchive.browseCached(file);
      assertEquals(file, browsedCached);
    }
    {
      final VfsFile resolved = (VfsFile) VfsArchive.resolve(uri);
      assertNotNull(resolved);
      assertEquals(normalizedUri, resolved.getUri());
    }
  }

  @Test
  public void testGzippedTrack() throws IOException {
    final Uri uri = getFile(app.zxtune.test.R.raw.gzipped, "gzipped");
    final VfsFile file = (VfsFile) VfsArchive.resolve(uri);
    assertNotNull(file);
    assertEquals("gzipped", file.getName());
    assertEquals("", file.getDescription());
    assertEquals("54KB", file.getSize());

    final Uri normalizedUri = file.getUri();
    final Uri subUri = normalizedUri.buildUpon().fragment("+unGZIP").build();
    try {
      VfsArchive.resolve(subUri);
      unreachable();
    } catch (Exception e) {
      assertNotNull("Thrown exception", e);
      assertEquals("No archive found", e.getMessage());
    }
    {
      final VfsFile browsed = (VfsFile) VfsArchive.browse(file);
      assertEquals(file, browsed);
    }
    {
      final VfsFile browsedCached = (VfsFile) VfsArchive.browseCached(file);
      assertEquals(file, browsedCached);
    }
    {
      final VfsFile resolved = (VfsFile) VfsArchive.resolve(uri);
      assertNotNull(resolved);
      assertEquals(normalizedUri, resolved.getUri());
    }
    {
      final VfsFile resolvedFile = (VfsFile) VfsArchive.resolve(subUri);
      assertNotNull(resolvedFile);
      assertEquals(subUri, resolvedFile.getUri());
      assertEquals("+unGZIP", resolvedFile.getName());
      assertEquals("sll3", resolvedFile.getDescription());
      assertEquals("2:33", resolvedFile.getSize());

      // no parent dir really, so just file itself
      final VfsFile resolvedFileParent = (VfsFile) resolvedFile.getParent();
      assertNotNull(resolvedFileParent);
      assertEquals(normalizedUri, resolvedFileParent.getUri());
    }
  }

  @Test
  public void testMultitrack() throws IOException {
    final Uri uri = getFile(app.zxtune.test.R.raw.multitrack, "multitrack");
    final VfsFile file = (VfsFile) VfsArchive.resolve(uri);
    assertNotNull(file);
    assertEquals("multitrack", file.getName());
    assertEquals("", file.getDescription());
    assertEquals("10KB", file.getSize());

    final Uri normalizedUri = file.getUri();
    final Uri subUri = normalizedUri.buildUpon().fragment("#2").build();
    try {
      VfsArchive.resolve(subUri);
      unreachable();
    } catch (Exception e) {
      assertNotNull("Thrown exception", e);
      assertEquals("No archive found", e.getMessage());
    }
    {
      final VfsDir browsed = (VfsDir) VfsArchive.browse(file);
      assertTrue(VfsArchive.checkIfArchive(browsed));
      assertEquals(normalizedUri, browsed.getUri());
    }
    {
      final VfsDir browsedCached = (VfsDir) VfsArchive.browseCached(file);
      assertTrue(VfsArchive.checkIfArchive(browsedCached));
      assertEquals(normalizedUri, browsedCached.getUri());
    }
    {
      final VfsDir resolved = (VfsDir) VfsArchive.resolve(uri);
      assertTrue(VfsArchive.checkIfArchive(resolved));
      assertEquals(normalizedUri, resolved.getUri());

      assertEquals(
          "{#1} (RoboCop 3 - Jeroen Tel) <3:56>\n" +
              "{#2} (RoboCop 3 - Jeroen Tel) <0:09>\n" +
              "{#3} (RoboCop 3 - Jeroen Tel) <0:16>\n" +
              "{#4} (RoboCop 3 - Jeroen Tel) <0:24>\n" +
              "{#5} (RoboCop 3 - Jeroen Tel) <0:24>\n" +
              "{#6} (RoboCop 3 - Jeroen Tel) <0:24>\n" +
              "{#7} (RoboCop 3 - Jeroen Tel) <0:12>\n" +
              "{#8} (RoboCop 3 - Jeroen Tel) <0:35>\n" +
              "{#9} (RoboCop 3 - Jeroen Tel) <0:01>\n" +
              "{#10} (RoboCop 3 - Jeroen Tel) <0:01>\n" +
              "{#11} (RoboCop 3 - Jeroen Tel) <0:01>\n" +
              "{#12} (RoboCop 3 - Jeroen Tel) <0:02>\n" +
              "{#13} (RoboCop 3 - Jeroen Tel) <0:01>\n" +
              "{#14} (RoboCop 3 - Jeroen Tel) <0:01>\n" +
              "{#15} (RoboCop 3 - Jeroen Tel) <0:01>\n" +
              "{#16} (RoboCop 3 - Jeroen Tel) <0:05>\n" +
              "{#17} (RoboCop 3 - Jeroen Tel) <0:01>\n" +
              "{#18} (RoboCop 3 - Jeroen Tel) <0:01>\n" +
              "{#19} (RoboCop 3 - Jeroen Tel) <0:03>\n" +
              "{#20} (RoboCop 3 - Jeroen Tel) <0:02>\n", listContent(resolved));
    }
    {
      final VfsFile resolvedFile = (VfsFile) VfsArchive.resolve(subUri);
      assertNotNull(resolvedFile);
      assertEquals(subUri, resolvedFile.getUri());
      assertEquals("#2", resolvedFile.getName());
      assertEquals("RoboCop 3 - Jeroen Tel", resolvedFile.getDescription());
      assertEquals("0:09", resolvedFile.getSize());

      // archive dir overrides file
      final VfsDir parentDir = (VfsDir) resolvedFile.getParent();
      assertNotNull(parentDir);
      assertEquals(normalizedUri, parentDir.getUri());
      assertTrue(VfsArchive.checkIfArchive(parentDir));

      final VfsDir parentDirParentDir = (VfsDir) parentDir.getParent();
      assertFalse(VfsArchive.checkIfArchive(parentDirParentDir));
    }
  }

  @Test
  public void testArchive() throws IOException {
    final Uri uri = getFile(app.zxtune.test.R.raw.archive, "archive");
    final VfsFile file = (VfsFile) VfsArchive.resolve(uri);
    assertNotNull(file);
    assertEquals("archive", file.getName());
    assertEquals("", file.getDescription());
    assertEquals("2.4KB", file.getSize());

    final Uri normalizedUri = file.getUri();
    final Uri subFileUri = normalizedUri.buildUpon().fragment("auricom.pt3").build();
    try {
      VfsArchive.resolve(subFileUri);
      unreachable();
    } catch (Exception e) {
      assertNotNull("Thrown exception", e);
      assertEquals("No archive found", e.getMessage());
    }

    final Uri subDirUri = normalizedUri.buildUpon().fragment("coop-Jeffie").build();
    try {
      VfsArchive.resolve(subDirUri);
      unreachable();
    } catch (Exception e) {
      assertNotNull("Thrown exception", e);
      assertEquals("No archive found", e.getMessage());
    }
    {
      final VfsDir browsed = (VfsDir) VfsArchive.browse(file);
      assertTrue(VfsArchive.checkIfArchive(browsed));
      assertEquals(normalizedUri, browsed.getUri());
    }
    {
      final VfsDir browsedCached = (VfsDir) VfsArchive.browseCached(file);
      assertTrue(VfsArchive.checkIfArchive(browsedCached));
      assertEquals(normalizedUri, browsedCached.getUri());
    }
    {
      final VfsDir resolved = (VfsDir) VfsArchive.resolve(uri);
      assertTrue(VfsArchive.checkIfArchive(resolved));
      assertEquals(normalizedUri, resolved.getUri());

      assertEquals(
          "{auricom.pt3} (auricom - sclsmnn^mc ft frolic&fo(?). 2015) <1:13>\n" +
              "[coop-Jeffie] ()\n" +
              " {bass sorrow.pt3} (bass sorrow - scalesmann^mc & jeffie/bw, 2004) <2:10>\n", listContent(resolved));
    }
    {
      final VfsDir resolvedDir = (VfsDir) VfsArchive.resolve(subDirUri);
      assertNotNull(resolvedDir);
      assertEquals(subDirUri, resolvedDir.getUri());
      assertEquals("coop-Jeffie", resolvedDir.getName());
      assertEquals("", resolvedDir.getDescription());
    }
    {
      final VfsFile resolvedFile = (VfsFile) VfsArchive.resolve(subFileUri);
      assertNotNull(resolvedFile);
      assertEquals(subFileUri, resolvedFile.getUri());
      assertEquals("auricom.pt3", resolvedFile.getName());
      assertEquals("auricom - sclsmnn^mc ft frolic&fo(?). 2015", resolvedFile.getDescription());
      assertEquals("1:13", resolvedFile.getSize());

      // archive dir overrides file
      final VfsDir parentDir = (VfsDir) resolvedFile.getParent();
      assertNotNull(parentDir);
      assertEquals(normalizedUri, parentDir.getUri());
      assertTrue(VfsArchive.checkIfArchive(parentDir));

      final VfsDir parentDirParentDir = (VfsDir) parentDir.getParent();
      assertFalse(VfsArchive.checkIfArchive(parentDirParentDir));
    }
  }

  @Test
  public void testForceResolve() throws IOException {
    {
      final Uri uri = getFile(app.zxtune.test.R.raw.gzipped, "gzipped");
      final VfsFile file = (VfsFile) VfsArchive.resolve(uri);

      final Uri normalizedUri = file.getUri();
      final Uri subUri = normalizedUri.buildUpon().fragment("+unGZIP").build();
      {
        final VfsFile resolvedFile = (VfsFile) VfsArchive.resolveForced(subUri,
            StubProgressCallback.instance());
        assertNotNull(resolvedFile);
        assertEquals(subUri, resolvedFile.getUri());
        assertEquals("+unGZIP", resolvedFile.getName());
        assertEquals("sll3", resolvedFile.getDescription());
        assertEquals("2:33", resolvedFile.getSize());

        // no parent dir really, so just file itself
        final VfsFile resolvedFileParent = (VfsFile) resolvedFile.getParent();
        assertNotNull(resolvedFileParent);
        assertEquals(normalizedUri, resolvedFileParent.getUri());
      }
    }
    {
      final Uri uri = getFile(app.zxtune.test.R.raw.multitrack, "multitrack");
      final VfsFile file = (VfsFile) VfsArchive.resolve(uri);
      assertNotNull(file);

      final Uri normalizedUri = file.getUri();
      final Uri subUri = normalizedUri.buildUpon().fragment("#2").build();
      {
        final VfsFile resolvedFile = (VfsFile) VfsArchive.resolveForced(subUri,
            StubProgressCallback.instance());
        assertNotNull(resolvedFile);
        assertEquals(subUri, resolvedFile.getUri());
        assertEquals("#2", resolvedFile.getName());
        assertEquals("RoboCop 3 - Jeroen Tel", resolvedFile.getDescription());
        assertEquals("0:09", resolvedFile.getSize());

        // archive dir overrides file
        final VfsDir parentDir = (VfsDir) resolvedFile.getParent();
        assertNotNull(parentDir);
        assertEquals(normalizedUri, parentDir.getUri());
        assertTrue(VfsArchive.checkIfArchive(parentDir));

        final VfsDir parentDirParentDir = (VfsDir) parentDir.getParent();
        assertFalse(VfsArchive.checkIfArchive(parentDirParentDir));
      }
    }
    {
      final Uri uri = getFile(app.zxtune.test.R.raw.archive, "archive");
      final VfsFile file = (VfsFile) VfsArchive.resolve(uri);
      assertNotNull(file);

      final Uri normalizedUri = file.getUri();
      final Uri subFileUri = normalizedUri.buildUpon().fragment("auricom.pt3").build();
      {
        final VfsFile resolvedFile = (VfsFile) VfsArchive.resolveForced(subFileUri,
            StubProgressCallback.instance());
        assertNotNull(resolvedFile);
        assertEquals(subFileUri, resolvedFile.getUri());
        assertEquals("auricom.pt3", resolvedFile.getName());
        assertEquals("auricom - sclsmnn^mc ft frolic&fo(?). 2015", resolvedFile.getDescription());
        assertEquals("1:13", resolvedFile.getSize());

        // archive dir overrides file
        final VfsDir parentDir = (VfsDir) resolvedFile.getParent();
        assertNotNull(parentDir);
        assertEquals(normalizedUri, parentDir.getUri());
        assertTrue(VfsArchive.checkIfArchive(parentDir));

        final VfsDir parentDirParentDir = (VfsDir) parentDir.getParent();
        assertFalse(VfsArchive.checkIfArchive(parentDirParentDir));
      }
    }
    {
      final Uri uri = getFile(app.zxtune.test.R.raw.archive, "archive2");
      final VfsFile file = (VfsFile) VfsArchive.resolve(uri);
      assertNotNull(file);

      final Uri normalizedUri = file.getUri();
      final Uri subDirUri = normalizedUri.buildUpon().fragment("coop-Jeffie").build();
      {
        final VfsDir resolvedDir = (VfsDir) VfsArchive.resolveForced(subDirUri,
            StubProgressCallback.instance());
        assertNotNull(resolvedDir);
        assertEquals(subDirUri, resolvedDir.getUri());
        assertEquals("coop-Jeffie", resolvedDir.getName());
        assertEquals("", resolvedDir.getDescription());
      }
    }
  }

  private static String listContent(VfsDir dir) {
    final StringBuilder list = new StringBuilder();
    listDir(dir, list, "");
    return list.toString();
  }

  private static void listDir(VfsDir input, final StringBuilder list, final String tab) {
    try {
      input.enumerate(new VfsDir.Visitor() {
        @Override
        public void onItemsCount(int count) {
        }

        @Override
        public void onDir(VfsDir dir) {
          list.append(String.format("%s[%s] (%s)\n", tab, dir.getName(), dir.getDescription()));
          listDir(dir, list, tab + " ");
        }

        @Override
        public void onFile(VfsFile file) {
          list.append(String.format("%s{%s} (%s) <%s>\n", tab, file.getName(), file.getDescription(), file.getSize()));
        }
      });
    } catch (Exception e) {
      list.append(e);
    }
  }
}
