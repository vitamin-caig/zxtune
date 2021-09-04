package app.zxtune.fs.amp;

import static org.junit.Assert.assertEquals;
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

import app.zxtune.fs.http.HttpProvider;
import app.zxtune.fs.http.HttpProviderFactory;
import app.zxtune.fs.http.MultisourceHttpProvider;

@RunWith(AndroidJUnit4.class)
public class CatalogTest {

  protected RemoteCatalog catalog;

  @Before
  public void setUp() {
    final Context ctx = InstrumentationRegistry.getInstrumentation().getTargetContext();
    final HttpProvider http = HttpProviderFactory.createProvider(ctx);
    catalog = new RemoteCatalog(new MultisourceHttpProvider(http));
  }

  private static <T> void check(int minSize, ArrayList<T> real, T[] ref) {
    assertTrue(real.size() >= minSize);
    for (T obj : ref) {
      assertTrue("Not found " + obj, contains(real, obj));
    }
  }

  private static <T> boolean contains(ArrayList<T> list, T obj) {
    for (T e : list) {
      if (matches(e, obj)) {
        return true;
      }
    }
    return false;
  }

  private static <T> boolean matches(T lh, T rh) {
    if (lh instanceof Group) {
      return matches((Group) lh, (Group) rh);
    } else if (lh instanceof Author) {
      return matches((Author) lh, (Author) rh);
    } else if (lh instanceof Track) {
      return matches((Track) lh, (Track) rh);
    } else {
      Assert.fail("Unknown type " + lh.getClass());
      return false;
    }
  }

  private static boolean matches(Group lh, Group rh) {
    return lh.id == rh.id && lh.name.equals(rh.name);
  }

  private static boolean matches(Author lh, Author rh) {
    return lh.id == rh.id && lh.handle.equals(rh.handle) && lh.realName.equals(rh.realName);
  }

  private static boolean matches(Track lh, Track rh) {
    return lh.id == rh.id && lh.filename.equals(rh.filename) && lh.size == rh.size;
  }

  @Test
  public void testGroups() throws IOException {
    final Group[] checkpoints = {
        new Group(6018, "0-Dezign"),
        new Group(6013, "ABSENCE"),
        // multiline
        new Group(677, "Anexia"),
        // utf-8
        new Group(9016, "Åltalános Védelmi Hiba (AHV)")
    };

    final ArrayList<Group> result = new ArrayList<>();
    catalog.queryGroups(new Catalog.GroupsVisitor() {
      @Override
      public void accept(Group obj) {
        result.add(obj);
      }
    });

    check(8825, result, checkpoints);
  }

  @Test
  public void testAuthorsByHandle() throws IOException {
    final Author[] checkpoints = {
        // first
        new Author(9913, "00d", "Petteri Kuittinen"),
        // pivot
        new Author(10, "4-Mat", "Matthew Simmonds"),
        // n/a in real name -> empty
        new Author(15226, "808-Style", ""),
        // utf-8
        new Author(13302, "8bitbubsy", "Olav Sørensen")
    };

    final AuthorsChecker checker = new AuthorsChecker();
    catalog.queryAuthors("0-9", checker);
    checker.check(46, checkpoints);
  }

  @Test
  public void testAuthorsByCountry() throws IOException {
    final Author[] checkpoints = {
        // first
        new Author(13606, "Agent Orange", "Oleg Sharonov"),
        // next page
        new Author(2142, "Doublestar", "Victor Shimyakov"),
        // last one
        new Author(8482, "Zyz", "")
    };

    final AuthorsChecker checker = new AuthorsChecker();
    catalog.queryAuthors(new Country(44, "Russia"), checker);
    checker.check(200, checkpoints);
  }

  @Test
  public void testAuthorsByGroup() throws IOException {
    final Author[] checkpoints = {
        // first
        new Author(14354, "BeRo", "Benjamin Rosseaux"),
        // last
        new Author(8120, "Wayfinder", "Sebastian Grillmaier")
    };

    final AuthorsChecker checker = new AuthorsChecker();
    catalog.queryAuthors(new Group(1770, "Farbrausch"), checker);
    checker.check(11, checkpoints);
  }

  private static class AuthorsChecker extends Catalog.AuthorsVisitor {
    private final ArrayList<Author> result = new ArrayList<>();

    @Override
    public void accept(Author obj) {
      result.add(obj);
    }

    final void check(int size, Author[] checkpoints) {
      CatalogTest.check(size, result, checkpoints);
    }
  }

  @Test
  public void testAuthorsTracks() throws IOException {
    final Track[] checkpoints = {
        // first
        new Track(15892, "egometriosporasie", 186),
        // instead unavailable
        new Track(15934, "un peu + a l'ouest", 309),
        // last
        new Track(15804, "yapleindmondalagas", 190)
    };

    final ArrayList<Track> result = new ArrayList<>();
    catalog.queryTracks(new Author(2085, "doh", "Nicolas Dessesart"), new Catalog.TracksVisitor() {
      @Override
      public void accept(Track obj) {
        result.add(obj);
      }
    });

    check(226 - 59, result, checkpoints);
  }

  @Test
  public void testSearch() throws IOException {
    final Track[] tracksCheckpoints = {
        // first
        new Track(121325, "\"bzzzt-chip 4\"", 7),
        // last
        new Track(22313, "zzzzzzzzz", 50)
    };

    final Author[] authorsCheckpoints = {
        // first
        new Author(8462, "Zuo", ""),
        // for multiple authors first is taken and merged into previous
        new Author(8071, "Voyce", ""),
        // last
        new Author(1669, "Daze", "")
    };

    final ArrayList<Track> tracks = new ArrayList<>();
    final ArrayList<Author> authors = new ArrayList<>();
    catalog.findTracks("zzz", new Catalog.FoundTracksVisitor() {
      @Override
      public void accept(Author author, Track track) {
        tracks.add(track);
        // simple uniq
        if (authors.isEmpty() || authors.get(authors.size() - 1).id != author.id) {
          authors.add(author);
        }
      }
    });

    check(52, tracks, tracksCheckpoints);
    check(42, authors, authorsCheckpoints);
  }
}
