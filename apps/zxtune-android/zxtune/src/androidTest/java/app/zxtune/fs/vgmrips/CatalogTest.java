package app.zxtune.fs.vgmrips;

import android.content.Context;

import androidx.collection.SparseArrayCompat;
import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;
import java.util.ArrayList;
import java.util.concurrent.TimeUnit;

import app.zxtune.TimeStamp;
import app.zxtune.fs.ProgressCallback;
import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.HttpProviderFactory;
import app.zxtune.fs.http.MultisourceHttpProvider;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;

@RunWith(AndroidJUnit4.class)
public class CatalogTest {

  protected Catalog catalog;

  @Before
  public void setUp() {
    final Context ctx = InstrumentationRegistry.getInstrumentation().getTargetContext();
    final HttpProvider http = HttpProviderFactory.createProvider(ctx);
    catalog = new RemoteCatalog(new MultisourceHttpProvider(http));
  }

  @Test
  public void testCompanies() throws IOException {
    final GroupsChecker checker = new GroupsChecker();
    catalog.companies().query(checker);

    final SparseArrayCompat<Group> checkpoints = new SparseArrayCompat<>();
    //first
    checkpoints.append(0, new Group("konami", "Konami", 202));
    checkpoints.append(101, new Group("naxat-soft", "Naxat Soft", 4));
    checkpoints.append(303, new Group("c-s-ware", "C's Ware", 1));
    checkpoints.append(405, new Group("crea-tech", "Crea-Tech", 1));
    checker.check(514, checkpoints);
  }

  @Test
  public void testCompanyPacks() throws IOException {
    final PacksChecker checker = new PacksChecker();
    catalog.companies().queryPacks("odyssey-software", checker, checker);

    final SparseArrayCompat<Pack> checkpoints = new SparseArrayCompat<>();
    //first
    checkpoints.append(0, makePack("solitaire-nes", "Solitaire", 4, 25, 3));

    checker.check(4, checkpoints);
    checker.checkProgress(4, 4);
  }

  @Test
  public void testComposers() throws IOException {
    final GroupsChecker checker = new GroupsChecker();
    catalog.composers().query(checker);

    final SparseArrayCompat<Group> checkpoints = new SparseArrayCompat<>();
    //first
    checkpoints.append(0, new Group("konami-kukeiha-club", "Konami Kukeiha Club", 47));
    checkpoints.append(502, new Group("mariko-egawa", "Mariko Egawa", 2));
    //last
    checkpoints.append(1457, new Group("zuntata-sound-team", "Zuntata Sound Team", 1));
    checker.check(1458, checkpoints);
  }

  @Test
  public void testComposerPacks() throws IOException {
    final PacksChecker checker = new PacksChecker();
    catalog.composers().queryPacks("yoko-shimomura", checker, checker);

    final SparseArrayCompat<Pack> checkpoints = new SparseArrayCompat<>();
    //first
    checkpoints.append(0, makePack("street-fighter-ii-champion-edition-cp-system", "Street " +
        "Fighter II: Champion Edition", 47, 45, 13));

    checker.check(14, checkpoints);
    checker.checkProgress(14, 14);
  }

  @Test
  public void testChips() throws IOException {
    final GroupsChecker checker = new GroupsChecker();
    catalog.chips().query(checker);

    final SparseArrayCompat<Group> checkpoints = new SparseArrayCompat<>();
    //first
    checkpoints.append(0, new Group("nes-apu", "NES APU", 427));
    //last
    checkpoints.append(47, new Group("vrc7", "VRC7", 1));
    checker.check(48, checkpoints);
  }

  @Test
  public void testChipPacks() throws IOException {
    final PacksChecker checker = new PacksChecker();
    catalog.chips().queryPacks("saa1099", checker, checker);

    final SparseArrayCompat<Pack> checkpoints = new SparseArrayCompat<>();
    //first
    checkpoints.append(0, makePack("sound-blaster-series-demo-songs-ibm-pc-xt-at", "Sound Blaster" +
        " Series Demo Songs", 17, 35, 4));

    checker.check(4, checkpoints);
    checker.checkProgress(4, 4);
  }


  @Test
  public void testSystems() throws IOException {
    final GroupsChecker checker = new GroupsChecker();
    catalog.systems().query(checker);

    final SparseArrayCompat<Group> checkpoins = new SparseArrayCompat<>();
    //first
    checkpoins.append(0, new Group("nintendo/family-computer", "Family Computer",
        327));
    checkpoins.append(10, new Group("ascii/msx", "MSX", 76));
    //last
    //checkpoins.append(182, new System("snk/neo-geo-pocket", "Neo Geo Pocket", 1));
    checker.check(183, checkpoins);
  }

  @Test
  public void testSystemPacks() throws IOException {
    final PacksChecker checker = new PacksChecker();
    catalog.systems().queryPacks("sinclair/zx-spectrum-128", checker, checker);

    final SparseArrayCompat<Pack> checkpoints = new SparseArrayCompat<>();
    //first
    checkpoints.append(0, makePack("the-ninja-warriors-zx-spectrum-128", "The Ninja Warriors",
        1, 45, 8));
    //last
    checkpoints.append(31, makePack("altered-beast-zx-spectrum-128", "Altered Beast",
        5, 15, 4));
    checker.check(32, checkpoints);
    checker.checkProgress(32, 32);
  }

  @Test
  public void testPackTracks() throws IOException {
    final TracksChecker checker = new TracksChecker();
    final Pack pack = catalog.findPack("the-scheme-nec-pc-8801-opna", checker);

    comparePacks(makePack("the-scheme-nec-pc-8801-opna", "The Scheme", 17, 45, 32), pack);

    final SparseArrayCompat<Track> checkpoints = new SparseArrayCompat<>();
    //first
    checkpoints.append(0, new Track(1, "Into The Lair", TimeStamp.createFrom(54,
        TimeUnit.SECONDS), "Other/The_Scheme_(NEC_PC-8801,_OPNA)/01 Into The Lair.vgz"));
    //last
    checkpoints.append(16, new Track(17, "Theme of Gigaikotsu", TimeStamp.createFrom(121,
        TimeUnit.SECONDS), "Other/The_Scheme_(NEC_PC-8801,_OPNA)/17 Theme of Gigaikotsu.vgz"));
    checker.check(17, checkpoints);
  }

  private static Pack makePack(String id, String title, int songs, int score, int ratings) {
    final Pack result = new Pack(id, title);
    result.songs = songs;
    result.score = score;
    result.ratings = ratings;
    return result;
  }

  private static class GroupsChecker implements Catalog.Visitor<Group> {

    final ArrayList<Group> result = new ArrayList<>();

    @Override
    public void accept(Group obj) {
      assertEquals(obj.id, obj.id.trim());
      assertEquals(obj.title, obj.title.trim());
      assertNotEquals(0, obj.packs);
      result.add(obj);
    }

    final void check(int size, SparseArrayCompat<Group> checkpoints) {
      assertEquals(size, result.size());
      for (int i = 0; i < checkpoints.size(); ++i) {
        final int pos = checkpoints.keyAt(i);
        final Group ref = checkpoints.valueAt(i);
        final Group real = result.get(pos);
        assertEquals(ref.id, real.id);
        assertEquals(ref.title, real.title);
        assertEquals(ref.packs, real.packs);
      }
    }
  }

  private static void comparePacks(Pack ref, Pack real) {
    assertEquals(ref.id, real.id);
    assertEquals(ref.title, real.title);
    assertEquals(ref.songs, real.songs);
    assertEquals(ref.score, real.score);
    assertEquals(ref.ratings, real.ratings);
  }

  private static class PacksChecker implements Catalog.Visitor<Pack>, ProgressCallback {

    final ArrayList<Pack> result = new ArrayList<>();
    final int[] lastProgress = {-1, -1};

    @Override
    public void accept(Pack obj) {
      assertEquals(obj.id, obj.id.trim());
      assertEquals(obj.title, obj.title.trim());
      result.add(obj);
    }

    final void check(int size, SparseArrayCompat<Pack> checkpoints) {
      assertEquals(size, result.size());
      for (int i = 0; i < checkpoints.size(); ++i) {
        final int pos = checkpoints.keyAt(i);
        final Pack ref = checkpoints.valueAt(i);
        final Pack real = result.get(pos);
        comparePacks(ref, real);
      }
    }

    @Override
    public void onProgressUpdate(int done, int total) {
      lastProgress[0] = done;
      lastProgress[1] = total;
    }

    final void checkProgress(int done, int total) {
      assertEquals(done, lastProgress[0]);
      assertEquals(total, lastProgress[1]);
    }
  }

  private static class TracksChecker implements Catalog.Visitor<Track> {

    final ArrayList<Track> result = new ArrayList<>();

    @Override
    public void accept(Track obj) {
      assertEquals(obj.title, obj.title.trim());
      assertEquals(obj.location, obj.location.trim());
      result.add(obj);
    }

    final void check(int size, SparseArrayCompat<Track> checkpoints) {
      assertEquals(size, result.size());
      for (int i = 0; i < checkpoints.size(); ++i) {
        final int pos = checkpoints.keyAt(i);
        final Track ref = checkpoints.valueAt(i);
        final Track real = result.get(pos);
        assertEquals(ref.number, real.number);
        assertEquals(ref.title, real.title);
        assertEquals(ref.duration, real.duration);
        assertEquals(ref.location, real.location);
      }
    }
  }
}
