package app.zxtune.fs.modland;

import static org.junit.Assert.assertEquals;

import android.content.Context;

import androidx.collection.SparseArrayCompat;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;
import java.util.ArrayList;

import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.HttpProviderFactory;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.utils.ProgressCallback;

@RunWith(AndroidJUnit4.class)
public class CatalogTest {

  protected RemoteCatalog catalog;

  @Before
  public void setUp() {
    final Context ctx = InstrumentationRegistry.getInstrumentation().getTargetContext();
    final HttpProvider http = HttpProviderFactory.createProvider(ctx);
    catalog = new RemoteCatalog(new MultisourceHttpProvider(http));
  }

  @Test
  public void testAuthors() throws IOException {
    final GroupsChecker checker = new GroupsChecker();
    catalog.getAuthors().queryGroups("#", checker, checker);

    final SparseArrayCompat<Group> checkpoints = new SparseArrayCompat<>();
    // first
    checkpoints.append(0, new Group(25189, "$4753 Softcopy", 33));
    // last
    checkpoints.append(91, new Group(13177, "9Nobo8", 1));
    checker.checkGroups(92, checkpoints);
    checker.checkProgress(92, 92);
  }

  @Test
  public void testAuthorsTracks() throws IOException {
    final TracksChecker checker = new TracksChecker();
    catalog.getAuthors().queryTracks(374/*4-Mat*/, checker, checker);

    final SparseArrayCompat<Track> checkpoints = new SparseArrayCompat<>();
    // first
    checkpoints.append(0, new Track("/pub/modules/Nintendo%20SPC/4-Mat/Micro%20Machines/01%20" +
        "-%20main%20theme.spc", 66108));
    // last (100th really)
    checkpoints.append(99, new Track("/pub/modules/Fasttracker%202/4-Mat/blade1.xm", 24504));
    checker.checkTracks(100, checkpoints);
    // two full pages processed
    checker.checkProgress(80, 744);
  }

  @Test
  public void testCollections() throws IOException {
    final GroupsChecker checker = new GroupsChecker();
    catalog.getCollections().queryGroups("O", checker, checker);

    final SparseArrayCompat<Group> checkpoints = new SparseArrayCompat<>();
    // first
    checkpoints.append(0, new Group(6282, "O&F", 10));
    // last
    checkpoints.append(115, new Group(6422, "Ozzyoss", 5));
    checker.checkGroups(116, checkpoints);
    checker.checkProgress(116, 116);
  }

  @Test
  public void testFormats() throws IOException {
    final GroupsChecker checker = new GroupsChecker();
    catalog.getFormats().queryGroups("M", checker, checker);

    final SparseArrayCompat<Group> checkpoints = new SparseArrayCompat<>();
    // first
    checkpoints.append(0, new Group(212, "Mad Tracker 2", 93));
    // last
    checkpoints.append(26, new Group(268, "MVX Module", 12));
    // single page
    checker.checkGroups(27, checkpoints);
    checker.checkProgress(27, 27);
  }

  private static class GroupsChecker implements Catalog.GroupsVisitor, ProgressCallback {

    final ArrayList<Group> result = new ArrayList<>();
    final int[] lastProgress = {-1, -1};

    @Override
    public void onProgressUpdate(int done, int total) {
      lastProgress[0] = done;
      lastProgress[1] = total;
    }

    @Override
    public void setCountHint(int hint) {}

    @Override
    public void accept(Group obj) {
      result.add(obj);
    }

    final void checkGroups(int size, SparseArrayCompat<Group> checkpoints) {
      assertEquals(size, result.size());
      for (int i = 0; i < checkpoints.size(); ++i) {
        final int pos = checkpoints.keyAt(i);
        final Group ref = checkpoints.valueAt(i);
        final Group real = result.get(pos);
        assertEquals(ref.getId(), real.getId());
        assertEquals(ref.getName(), real.getName());
        assertEquals(ref.getTracks(), real.getTracks());
      }
    }

    final void checkProgress(int done, int total) {
      assertEquals(done, lastProgress[0]);
      assertEquals(total, lastProgress[1]);
    }
  }

  private static class TracksChecker implements Catalog.TracksVisitor, ProgressCallback {

    final ArrayList<Track> result = new ArrayList<>();
    final int[] lastProgress = {-1, -1};

    @Override
    public void onProgressUpdate(int done, int total) {
      lastProgress[0] = done;
      lastProgress[1] = total;
    }

    @Override
    public void setCountHint(int hint) {}

    @Override
    public boolean accept(Track obj) {
      result.add(obj);
      return result.size() < 100;
    }

    final void checkTracks(int size, SparseArrayCompat<Track> checkpoints) {
      assertEquals(size, result.size());
      for (int i = 0; i < checkpoints.size(); ++i) {
        final int pos = checkpoints.keyAt(i);
        final Track ref = checkpoints.valueAt(i);
        final Track real = result.get(pos);
        assertEquals(ref.filename, real.filename);
        assertEquals(ref.path, real.path);
        assertEquals(ref.size, real.size);
        assertEquals(ref.id, real.id);
      }
    }

    final void checkProgress(int done, int total) {
      assertEquals(done, lastProgress[0]);
      assertEquals(total, lastProgress[1]);
    }
  }
}
