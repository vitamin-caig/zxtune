package app.zxtune.playback;

import android.content.res.Resources;
import android.net.Uri;
import androidx.annotation.RawRes;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import app.zxtune.fs.Vfs;
import app.zxtune.io.Io;
import app.zxtune.io.TransactionalOutputStream;
import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.BufferedInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;

import static org.junit.Assert.*;

@RunWith(AndroidJUnit4.class)
public class FileIteratorTest {

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
    return Vfs.resolve(Uri.fromFile(result)).getUri();
  }

  @Before
  public void setUp() {
    resources = InstrumentationRegistry.getContext().getResources();
    tmpDir = new File(System.getProperty("java.io.tmpdir", "."), String.format("VfsArchiveTest/%d",
        System.currentTimeMillis()));
  }

  @After
  public void tearDown() {
    for (File f : tmpDir.listFiles()) {
      f.delete();
    }
    tmpDir.delete();
  }

  @Test
  public void testTrack() throws Exception {
    getFile(app.zxtune.test.R.raw.multitrack, "0");
    final Uri track = getFile(app.zxtune.test.R.raw.track, "1");
    getFile(app.zxtune.test.R.raw.gzipped, "2");

    final FileIterator iter = FileIterator.create(InstrumentationRegistry.getContext(), track);
    final ArrayList<PlayableItem> items = new ArrayList<>();
    do {
      items.add(iter.getItem());
    } while (iter.next());

    assertEquals(2, items.size());
    assertEquals("AsSuRed ... Hi! My Frends ...", items.get(0).getTitle());
    assertEquals("sll3", items.get(1).getTitle());
  }

  @Test
  public void testSingleArchived() throws Exception {
    getFile(app.zxtune.test.R.raw.multitrack, "0");
    final Uri gzipped = getFile(app.zxtune.test.R.raw.gzipped, "1");
    getFile(app.zxtune.test.R.raw.track, "2");

    final FileIterator iter = FileIterator.create(InstrumentationRegistry.getContext(), gzipped.buildUpon().fragment(
        "+unGZIP").build());
    final ArrayList<PlayableItem> items = new ArrayList<>();
    do {
      items.add(iter.getItem());
    } while (iter.next());

    assertEquals(2, items.size());
    assertEquals("sll3", items.get(0).getTitle());
    assertEquals("AsSuRed ... Hi! My Frends ...", items.get(1).getTitle());
  }

  @Test
  public void testArchivedElement() throws Exception {
    getFile(app.zxtune.test.R.raw.multitrack, "0");
    final Uri archived = getFile(app.zxtune.test.R.raw.archive, "1");
    getFile(app.zxtune.test.R.raw.track, "2");

    final FileIterator iter = FileIterator.create(InstrumentationRegistry.getContext(), archived.buildUpon().fragment(
        "coop-Jeffie/bass sorrow.pt3").build());
    final ArrayList<PlayableItem> items = new ArrayList<>();
    do {
      items.add(iter.getItem());
    } while (iter.next());

    assertEquals(1, items.size());
    assertEquals("bass sorrow", items.get(0).getTitle());
  }

  @Test
  public void testMultitrackElement() throws Exception {
    getFile(app.zxtune.test.R.raw.gzipped, "0");
    final Uri multitrack = getFile(app.zxtune.test.R.raw.multitrack, "1");
    getFile(app.zxtune.test.R.raw.track, "2");

    final FileIterator iter = FileIterator.create(InstrumentationRegistry.getContext(), multitrack.buildUpon().fragment(
        "#2").build());
    final ArrayList<PlayableItem> items = new ArrayList<>();
    do {
      items.add(iter.getItem());
    } while (iter.next());

    assertEquals(19, items.size());
    assertEquals("RoboCop 3", items.get(0).getTitle());
  }
}
