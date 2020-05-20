package app.zxtune.fs.amp;

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

import static org.junit.Assert.assertEquals;

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
  public void testGroups() throws IOException {
    final ArrayList<Group> result = new ArrayList<>();
    catalog.queryGroups(new Catalog.GroupsVisitor() {
      @Override
      public void accept(Group obj) {
        result.add(obj);
      }
    });
    assertEquals(8756, result.size());

    final SparseArrayCompat<Group> checkpoints = new SparseArrayCompat<>();
    // several groups w/o title
    checkpoints.append(0, new Group(6130, ""));
    checkpoints.append(1, new Group(9703, ""));
    checkpoints.append(2, new Group(9846, ""));
    checkpoints.append(3, new Group(10001, ""));
    checkpoints.append(4, new Group(6018, "0-Dezign"));
    checkpoints.append(100, new Group(6013, "ABSENCE"));
    // multiline
    checkpoints.append(444, new Group(677, "Anexia"));
    // utf-8
    checkpoints.append(8754, new Group(9016, "Åltalános Védelmi Hiba (AHV)"));

    for (int i = 0; i < checkpoints.size(); ++i) {
      final int pos = checkpoints.keyAt(i);
      final Group ref = checkpoints.valueAt(i);
      final Group real = result.get(pos);
      assertEquals(ref.id, real.id);
      assertEquals(ref.name, real.name);
    }
  }

  @Test
  public void testAuthorsByHandle() throws IOException {
    final SparseArrayCompat<Author> checkpoints = new SparseArrayCompat<>();
    // first
    checkpoints.append(0, new Author(9913, "00d", "Petteri Kuittinen"));
    // pivot
    checkpoints.append(26, new Author(10,"4-Mat", "Matthew Simmonds"));
    // n/a in real name -> empty
    checkpoints.append(41, new Author(15226, "808-Style", ""));
    // utf-8
    checkpoints.append(42, new Author(13302, "8bitbubsy", "Olav Sørensen"));

    final AuthorsChecker checker = new AuthorsChecker();
    catalog.queryAuthors("0-9", checker);
    checker.check(46, checkpoints);
  }

  @Test
  public void testAuthorsByCountry() throws IOException {
    final SparseArrayCompat<Author> checkpoints = new SparseArrayCompat<>();
    // first
    checkpoints.append(0, new Author(13606, "Agent Orange", "Oleg Sharonov"));
    // next page
    checkpoints.append(50, new Author(2142, "Doublestar", "Victor Shimyakov"));
    // last one
    checkpoints.append(197, new Author(8482, "Zyz", ""));

    final AuthorsChecker checker = new AuthorsChecker();
    catalog.queryAuthors(new Country(44, "Russia"), checker);
    checker.check(198, checkpoints);
  }

  @Test
  public void testAuthorsByGroup() throws IOException {
    final SparseArrayCompat<Author> checkpoints = new SparseArrayCompat<>();
    // first
    checkpoints.append(0, new Author(14354, "BeRo", "Benjamin Rosseaux"));
    // last
    checkpoints.append(10, new Author(8120, "Wayfinder", "Sebastian Grillmaier"));

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

    final void check(int size, SparseArrayCompat<Author> checkpoints) {
      checkAuthors(size, checkpoints, result);
    }
  }

  private static void checkAuthors(int size, SparseArrayCompat<Author> checkpoints,
                            ArrayList<Author> result) {
    assertEquals(size, result.size());
    for (int i = 0; i < checkpoints.size(); ++i) {
      final int pos = checkpoints.keyAt(i);
      final Author ref = checkpoints.valueAt(i);
      final Author real = result.get(pos);
      assertEquals(ref.id, real.id);
      assertEquals(ref.handle, real.handle);
      assertEquals(ref.realName, real.realName);
    }
  }

  @Test
  public void testAuthorsTracks() throws IOException {
    final ArrayList<Track> result = new ArrayList<>();
    catalog.queryTracks(new Author(2085, "doh", "Nicolas Dessesart"), new Catalog.TracksVisitor() {
      @Override
      public void accept(Track obj) {
        result.add(obj);
      }
    });

    final SparseArrayCompat<Track> checkpoints = new SparseArrayCompat<>();
    // first
    checkpoints.append(0, new Track(15892, "egometriosporasie", 186));
    // instead unavailable
    checkpoints.append(12, new Track(15934, "un peu + a l'ouest", 309));
    // last
    checkpoints.append(166, new Track(15804, "yapleindmondalagas", 190));

    checkTracks(226 - 59, checkpoints, result);
  }

  private static void checkTracks(int size, SparseArrayCompat<Track> checkpoints,
                                  ArrayList<Track> result) {
    assertEquals(size, result.size());
    for (int i = 0; i < checkpoints.size(); ++i) {
      final int pos = checkpoints.keyAt(i);
      final Track ref = checkpoints.valueAt(i);
      final Track real = result.get(pos);
      assertEquals(ref.id, real.id);
      assertEquals(ref.filename, real.filename);
      assertEquals(ref.size, real.size);
    }
  }

  @Test
  public void testSearch() throws IOException {
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

    {
      final SparseArrayCompat<Track> checkpoints = new SparseArrayCompat<>();
      // first
      checkpoints.append(0, new Track(121325, "\"bzzzt-chip 4\"", 7));
      // last
      checkpoints.append(49, new Track(22313, "zzzzzzzzz", 50));

      checkTracks(50, checkpoints, tracks);
    }

    {
      final SparseArrayCompat<Author> checkpoints = new SparseArrayCompat<>();
      // first
      checkpoints.append(0, new Author(8462, "Zuo", ""));
      // for multiple authors first is taken and merged into previous
      checkpoints.append(10, new Author(8071, "Voyce", ""));
      // last
      checkpoints.append(39, new Author(1669, "Daze", ""));

      checkAuthors(40, checkpoints, authors);
    }
  }
}
