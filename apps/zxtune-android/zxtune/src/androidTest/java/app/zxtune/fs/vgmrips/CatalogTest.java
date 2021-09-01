package app.zxtune.fs.vgmrips;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotEquals;
import static org.junit.Assert.assertTrue;

import android.content.Context;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.IOException;
import java.util.ArrayList;

import app.zxtune.TimeStamp;
import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.HttpProviderFactory;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.utils.ProgressCallback;

@RunWith(AndroidJUnit4.class)
public class CatalogTest {

  protected Catalog catalog;

  @Before
  public void setUp() {
    final Context ctx = InstrumentationRegistry.getInstrumentation().getTargetContext();
    final HttpProvider http = HttpProviderFactory.createProvider(ctx);
    catalog = new RemoteCatalog(new MultisourceHttpProvider(http));
  }

  private static <T> void check(int expectedSize, T[] expectedSubset, ArrayList<T> actual) {
    assertEquals(expectedSize, actual.size());
    for (T expected : expectedSubset) {
      assertTrue("Not found " + expected, contains(actual, expected));
    }
  }

  private static <T> boolean contains(ArrayList<T> actual, T expected) {
    for (T e : actual) {
      if (matches(expected, e)) {
        return true;
      }
    }
    return false;
  }

  private static <T> boolean matches(T expected, T actual) {
    if (expected instanceof Group) {
      return matches((Group) expected, (Group) actual);
    } else if (expected instanceof Pack) {
      return matches((Pack) expected, (Pack) actual);
    } else if (expected instanceof Track) {
      return matches((Track) expected, (Track) actual);
    } else {
      Assert.fail("Unknown type " + expected.getClass());
      return false;
    }
  }

  // Match only visible in UI attributes
  private static boolean matches(Group expected, Group actual) {
    if (expected.id.equals(actual.id)) {
      Assert.assertEquals(expected.id, expected.title, actual.title);
      Assert.assertEquals(expected.id, expected.packs, actual.packs);
      return true;
    } else {
      return false;
    }
  }

  private static boolean matches(Pack expected, Pack actual) {
    if (expected.id.equals(actual.id)) {
      Assert.assertEquals(expected.id, expected.title, actual.title);
      //Assert.assertEquals(expected.id, expected.ratings, actual.ratings);
      //Assert.assertEquals(expected.id, expected.score, actual.score);
      Assert.assertEquals(expected.id, expected.songs, actual.songs);
      return true;
    } else {
      return false;
    }
  }

  private static boolean matches(Track expected, Track actual) {
    if (expected.location.equals(actual.location)) {
      Assert.assertEquals(expected.location, expected.title, actual.title);
      Assert.assertEquals(expected.location, expected.number, actual.number);
      Assert.assertEquals(expected.location, expected.duration, actual.duration);
      return true;
    } else {
      return false;
    }
  }

  @Test
  public void testCompanies() throws IOException {
    final Group[] checkpoints = {
        //first
        new Group("konami", "Konami", 218),
        new Group("naxat-soft", "Naxat Soft", 5),
        new Group("c-s-ware", "C's Ware", 4),
        new Group("crea-tech", "Crea-Tech", 1)
    };

    final GroupsChecker checker = new GroupsChecker();
    catalog.companies().query(checker);

    checker.check(542, checkpoints);
  }

  @Test
  public void testCompanyPacks() throws IOException {
    final Pack[] checkpoints = {
        // first
        makePack("solitaire-nes", "Solitaire", 4, 25, 3)
    };

    final PacksChecker checker = new PacksChecker();
    catalog.companies().queryPacks("odyssey-software", checker, checker);

    checker.check(4, checkpoints);
    checker.checkProgress(4, 4);
  }

  @Test
  public void testComposers() throws IOException {
    final Group[] checkpoints = {
        //first
        new Group("konami-kukeiha-club", "Konami Kukeiha Club", 61),
        new Group("mariko-egawa", "Mariko Egawa", 2),
        //last
        new Group("zuntata-sound-team", "Zuntata Sound Team", 1)
    };

    final GroupsChecker checker = new GroupsChecker();
    catalog.composers().query(checker);

    checker.check(1514, checkpoints);
  }

  @Test
  public void testComposerPacks() throws IOException {
    final Pack[] checkpoints = {
        // first
        makePack("street-fighter-ii-champion-edition-cp-system", "Street Fighter II: Champion Edition", 47, 45, 13)
    };

    final PacksChecker checker = new PacksChecker();
    catalog.composers().queryPacks("yoko-shimomura", checker, checker);

    checker.check(15, checkpoints);
    checker.checkProgress(15, 15);
  }

  @Test
  public void testChips() throws IOException {
    final Group[] checkpoints = {
        //first
        new Group("nes-apu", "NES APU", 436),
        //last
        new Group("vrc7", "VRC7", 1)
    };

    final GroupsChecker checker = new GroupsChecker();
    catalog.chips().query(checker);

    checker.check(49, checkpoints);
  }

  @Test
  public void testChipPacks() throws IOException {
    final Pack[] checkpoints = {
        //first
        makePack("sound-blaster-series-demo-songs-ibm-pc-xt-at", "Sound Blaster Series Demo Songs", 17, 30, 4)
    };

    final PacksChecker checker = new PacksChecker();
    catalog.chips().queryPacks("saa1099", checker, checker);

    checker.check(4, checkpoints);
    checker.checkProgress(4, 4);
  }


  @Test
  public void testSystems() throws IOException {
    final Group[] checkpoins = {
        //first
        new Group("nintendo/family-computer", "Family Computer",
            333),
        new Group("ascii/msx", "MSX", 89)
        //last
        //checkpoins.append(182, new System("snk/neo-geo-pocket", "Neo Geo Pocket", 1));
    };

    final GroupsChecker checker = new GroupsChecker();
    catalog.systems().query(checker);

    checker.check(184, checkpoins);
  }

  @Test
  public void testSystemPacks() throws IOException {
    final Pack[] checkpoints = {
        //first
        makePack("the-ninja-warriors-zx-spectrum-128", "The Ninja Warriors",
            1, 45, 8),
        //last
        makePack("altered-beast-zx-spectrum-128", "Altered Beast",
            5, 15, 4)
    };

    final PacksChecker checker = new PacksChecker();
    catalog.systems().queryPacks("sinclair/zx-spectrum-128", checker, checker);

    checker.check(32, checkpoints);
    checker.checkProgress(32, 32);
  }

  @Test
  public void testPackTracks() throws IOException {
    final Track[] checkpoints = {
        //first
        new Track(1, "Into The Lair", TimeStamp.fromSeconds(54), "Other/The_Scheme_(NEC_PC-8801,_OPNA)/01 Into The Lair.vgz"),
        //last
        new Track(17, "Theme of Gigaikotsu", TimeStamp.fromSeconds(121), "Other/The_Scheme_(NEC_PC-8801,_OPNA)/17 Theme of Gigaikotsu.vgz")
    };

    final TracksChecker checker = new TracksChecker();
    final Pack pack = catalog.findPack("the-scheme-nec-pc-8801-opna", checker);

    Assert.assertTrue(matches(makePack("the-scheme-nec-pc-8801-opna", "The Scheme", 17, 45, 34),
        pack));

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

    final void check(int size, Group[] checkpoints) {
      CatalogTest.check(size, checkpoints, result);
    }
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

    final void check(int size, Pack[] checkpoints) {
      CatalogTest.check(size, checkpoints, result);
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

    final void check(int size, Track[] checkpoints) {
      CatalogTest.check(size, checkpoints, result);
    }
  }
}
