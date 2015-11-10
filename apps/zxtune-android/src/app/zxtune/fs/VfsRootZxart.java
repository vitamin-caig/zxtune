/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;

import android.content.Context;
import android.net.Uri;
import android.util.SparseIntArray;
import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.fs.zxart.Author;
import app.zxtune.fs.zxart.Catalog;
import app.zxtune.fs.zxart.Identifier;
import app.zxtune.fs.zxart.Party;
import app.zxtune.fs.zxart.Track;

public class VfsRootZxart extends StubObject implements VfsRoot {

  private final static String TAG = VfsRootZxart.class.getName();

  private final Context context;
  private final Catalog catalog;
  private final GroupingDir groups[];

  public VfsRootZxart(Context context, HttpProvider http) {
    this.context = context;
    this.catalog = Catalog.create(context, http);
    this.groups = new GroupingDir[] {
        new AuthorsDir(),
        new PartiesDir(),
        new TopTracksDir()
    };
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_zxart_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_zxart_root_description);
  }

  @Override
  public Uri getUri() {
    return Identifier.forRoot().build();
  }

  @Override
  public VfsObject getParent() {
    return null;
  }

  @Override
  public Object getExtension(String id) {
    if (VfsExtensions.ICON_RESOURCE.equals(id)) {
      return R.drawable.ic_browser_vfs_zxart;
    } else if (VfsExtensions.SEARCH_ENGINE.equals(id) && catalog.searchSupported()) {
      //assume search by authors from root
      return new AuthorsSearchEngine();
    } else {
      return super.getExtension(id);
    }
  }

  @Override
  public void enumerate(Visitor visitor) {
    for (GroupingDir group : groups) {
      visitor.onDir(group);
    }
  }

  @Override
  public VfsObject resolve(Uri uri) throws IOException {
    try {
      if (Identifier.isFromRoot(uri)) {
        final List<String> path = uri.getPathSegments();
        return resolve(uri, path);
      }
    } catch (Exception e) {
      Log.d(TAG, e, "resolve(%s)", uri);
    }
    return null;
  }

  private VfsObject resolve(Uri uri, List<String> path) throws IOException {
    final String category = Identifier.findCategory(path);
    if (category == null) {
      return this;
    } else {
      for (GroupingDir group : groups) {
        if (category.equals(group.getPath())) {
          return group.resolve(uri, path);
        }
      }
    }
    return null;
  }

  private abstract class GroupingDir extends StubObject implements VfsDir {

    @Override
    public Uri getUri() {
      return Identifier.forCategory(getPath()).build();
    }

    abstract String getPath();

    abstract VfsObject resolve(Uri uri, List<String> path) throws IOException;
  }

  private class AuthorsDir extends GroupingDir {

    @Override
    public String getName() {
      return context.getString(R.string.vfs_zxart_authors_name);
    }

    @Override
    public String getDescription() {
      return context.getString(R.string.vfs_zxart_authors_description);
    }

    @Override
    public VfsObject getParent() {
      return VfsRootZxart.this;
    }

    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.SEARCH_ENGINE.equals(id) && catalog.searchSupported()) {
        return new AuthorsSearchEngine();
      } else {
        return super.getExtension(id);
      }
    }
    
    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryAuthors(new Catalog.AuthorsVisitor() {
        @Override
        public void setCountHint(int hint) {
          visitor.onItemsCount(hint);
        }

        @Override
        public void accept(Author obj) {
          visitor.onDir(new AuthorDir(obj));
        }
      });
    }

    @Override
    public String getPath() {
      return Identifier.CATEGORY_AUTHORS;
    }

    @Override
    public VfsObject resolve(Uri uri, List<String> path) throws IOException {
      // use plain resolving to avoid deep hierarchy
      // check most frequent cases first
      final Track track = Identifier.findAuthorTrack(uri, path);
      if (track != null) {
        return new AuthorTrackFile(uri, track);
      }
      final VfsObject dir = resolveAuthorDir(uri, path);
      return dir != null
          ? dir
          : this;
    }
  }

  private VfsObject resolveAuthorDir(Uri uri, List<String> path) {
    final Author author = Identifier.findAuthor(uri, path);
    if (author == null) {
      return null;
    }
    final Integer year = Identifier.findAuthorYear(uri, path);
    return year != null
        ? new AuthorYearDir(author, year)
        : new AuthorDir(author);
  }

  private class AuthorDir extends StubObject implements VfsDir {

    private final Author author;

    AuthorDir(Author author) {
      this.author = author;
    }

    @Override
    public Uri getUri() {
      return Identifier.forAuthor(author).build();
    }

    @Override
    public String getName() {
      return author.nickname;
    }

    @Override
    public String getDescription() {
      return author.name;
    }

    @Override
    public VfsObject getParent() {
      return groups[0];// TODO
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      final SparseIntArray years = new SparseIntArray();

      catalog.queryAuthorTracks(author, new Catalog.TracksVisitor() {
        @Override
        public void accept(Track obj) {
          years.put(obj.year, 1 + years.get(obj.year));
        }
      });
      visitor.onItemsCount(years.size());
      for (int i = 0, lim = years.size(); i != lim; ++i) {
        // handle count when required
        visitor.onDir(new AuthorYearDir(author, years.keyAt(i)));
      }
    }
  }

  private class AuthorYearDir extends StubObject implements VfsDir {

    private final Author author;
    private final int year;

    AuthorYearDir(Author author, int year) {
      this.author = author;
      this.year = year;
    }

    @Override
    public Uri getUri() {
      return Identifier.forAuthor(author, year).build();
    }

    @Override
    public String getName() {
      return year != 0
          ? Integer.toString(year)
          : context.getString(R.string.vfs_zxart_unknown_year_name);
    }

    @Override
    public VfsObject getParent() {
      return new AuthorDir(author);
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryAuthorTracks(author, new Catalog.TracksVisitor() {
        @Override
        public void setCountHint(int size) {
          visitor.onItemsCount(size);
        }

        @Override
        public void accept(Track obj) {
          if (year == obj.year) {
            final Uri uri = Identifier.forTrack(Identifier.forAuthor(author, year), obj).build();
            visitor.onFile(new AuthorTrackFile(uri, obj));
          }
        }
      });
    }
  }

  private class AuthorTrackFile extends BaseTrackFile {

    public AuthorTrackFile(Uri uri, Track track) {
      super(uri, track);
    }

    @Override
    public VfsObject getParent() {
      final List<String> path = uri.getPathSegments();
      return resolveAuthorDir(uri, path);
    }
  }

  private class AuthorsSearchEngine implements VfsExtensions.SearchEngine {
    
    @Override
    public void find(String query, final Visitor visitor) throws IOException {
      catalog.findTracks(query, new Catalog.FoundTracksVisitor() {
        
        @Override
        public void accept(Author author, Track track) {
          final Uri uri = Identifier.forTrack(Identifier.forAuthor(author, track.year), track).build();
          visitor.onFile(new AuthorTrackFile(uri, track));
        }
      });
    }
  }
  
  private class PartiesDir extends GroupingDir {

    @Override
    public String getName() {
      return context.getString(R.string.vfs_zxart_parties_name);
    }

    @Override
    public String getDescription() {
      return context.getString(R.string.vfs_zxart_parties_description);
    }

    @Override
    public VfsObject getParent() {
      return VfsRootZxart.this;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      final SparseIntArray years = new SparseIntArray();
      catalog.queryParties(new Catalog.PartiesVisitor() {
        @Override
        public void accept(Party obj) {
          years.put(obj.year, 1 + years.get(obj.year));
        }
      });
      visitor.onItemsCount(years.size());
      for (int i = 0, lim = years.size(); i != lim; ++i) {
        visitor.onDir(new PartyYearDir(years.keyAt(i)));
      }
    }

    @Override
    public String getPath() {
      return Identifier.CATEGORY_PARTIES;
    }

    @Override
    public VfsObject resolve(Uri uri, List<String> path) throws IOException {
      final Track track = Identifier.findPartyTrack(uri, path);
      if (track != null) {
        return new PartyTrackFile(uri, track);
      }
      final VfsObject dir = resolvePartyDir(uri, path);
      return dir != null ? dir : this;
    }
  }

  private VfsObject resolvePartyDir(Uri uri, List<String> path) {
    final Party party = Identifier.findParty(uri, path);
    if (party == null) {
      final Integer year = Identifier.findPartiesYear(uri, path);
      return year != null
          ? new PartyYearDir(year)
          : null;
    }
    final String compo = Identifier.findPartyCompo(uri, path);
    return compo != null
        ? new PartyCompoDir(party, compo)
        : new PartyDir(party);
  }

  private class PartyYearDir extends StubObject implements VfsDir {

    private final int year;

    PartyYearDir(int year) {
      this.year = year;
    }

    @Override
    public Uri getUri() {
      return Identifier.forPartiesYear(year).build();
    }

    @Override
    public String getName() {
      return Integer.toString(year);
    }

    @Override
    public VfsObject getParent() {
      return groups[1]; // TODO
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryParties(new Catalog.PartiesVisitor() {
        @Override
        public void accept(Party obj) {
          if (obj.year == year) {
            visitor.onDir(new PartyDir(obj));
          }
        }
      });
    }
  }

  private class PartyDir extends StubObject implements VfsDir {

    private final Party party;

    PartyDir(Party party) {
      this.party = party;
    }

    @Override
    public Uri getUri() {
      return Identifier.forParty(party).build();
    }

    @Override
    public String getName() {
      return party.name;
    }

    @Override
    public VfsObject getParent() {
      return new PartyYearDir(party.year);
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      final HashSet<String> compos = new HashSet<String>();
      catalog.queryPartyTracks(party, new Catalog.TracksVisitor() {
        @Override
        public void accept(Track obj) {
          compos.add(obj.compo);
        }
      });
      visitor.onItemsCount(compos.size());
      for (String compo : compos) {
        visitor.onDir(new PartyCompoDir(party, compo));
      }
    }
  }

  enum CompoIdentifier {
    unknown(R.string.vfs_zxart_compo_unknown),
    standard(R.string.vfs_zxart_compo_standard),
    ay(R.string.vfs_zxart_compo_ay),
    beeper(R.string.vfs_zxart_compo_beeper),
    copyay(R.string.vfs_zxart_compo_copyay),
    nocopyay(R.string.vfs_zxart_compo_nocopyay),
    realtime(R.string.vfs_zxart_compo_realtime),
    realtimeay(R.string.vfs_zxart_compo_realtimeay),
    realtimebeeper(R.string.vfs_zxart_compo_realtimebeeper),
    out(R.string.vfs_zxart_compo_out),
    wild(R.string.vfs_zxart_compo_wild),
    experimental(R.string.vfs_zxart_compo_experimental),
    oldschool(R.string.vfs_zxart_compo_oldschool),
    mainstream(R.string.vfs_zxart_compo_mainstream),
    progressive(R.string.vfs_zxart_compo_progressive);

    private final int resourceId;

    private CompoIdentifier(int id) {
      this.resourceId = id;
    }

    final int getResource() {
      return resourceId;
    }

    static CompoIdentifier getId(String val) {
      try {
        return CompoIdentifier.valueOf(val);
      } catch (IllegalArgumentException e) {
        return CompoIdentifier.unknown;
      }
    }
  }

  // custom ordering by party place
  private class PartyCompoDir extends StubObject implements VfsDir, Comparator<VfsObject> {

    private final Party party;
    private final CompoIdentifier compo;

    PartyCompoDir(Party party, String compo) {
      this.party = party;
      this.compo = CompoIdentifier.getId(compo);
    }

    @Override
    public Uri getUri() {
      return Identifier.forPartyCompo(party, compo.name()).build();
    }

    @Override
    public String getName() {
      return context.getString(compo.getResource());
    }

    @Override
    public VfsObject getParent() {
      return new PartyDir(party);
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryPartyTracks(party, new Catalog.TracksVisitor() {
        @Override
        public void setCountHint(int size) {
          visitor.onItemsCount(size);
        }

        @Override
        public void accept(Track obj) {
          if (compo.name().equals(obj.compo)) {
            final Uri uri = Identifier.forTrack(Identifier.forPartyCompo(party, compo.name()), obj)
                .build();
            visitor.onFile(new PartyTrackFile(uri, obj));
          }
        }
      });
    }

    @Override
    public int compare(VfsObject lh, VfsObject rh) {
      final int lhPlace = ((BaseTrackFile) lh).module.partyplace;
      final int rhPlace = ((BaseTrackFile) rh).module.partyplace;
      return lhPlace == rhPlace
          ? 0
          : (lhPlace < rhPlace ? -1 : +1);
    }
  }

  private class PartyTrackFile extends BaseTrackFile {

    PartyTrackFile(Uri uri, Track module) {
      super(uri, module);
    }

    @Override
    public VfsObject getParent() {
      final List<String> path = uri.getPathSegments();
      return resolvePartyDir(uri, path);
    }

    @Override
    public String getSize() {
      return Integer.toString(module.partyplace);
    }
  }

  // custom ordering by votes desc
  private class TopTracksDir extends GroupingDir implements Comparator<VfsObject> {

    @Override
    public String getName() {
      return context.getString(R.string.vfs_zxart_toptracks_name);
    }

    @Override
    public String getDescription() {
      return context.getString(R.string.vfs_zxart_toptracks_description);
    }

    @Override
    public VfsObject getParent() {
      return VfsRootZxart.this;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryTopTracks(100, new Catalog.TracksVisitor() {
        @Override
        public void setCountHint(int size) {
          visitor.onItemsCount(size);
        }

        @Override
        public void accept(Track obj) {
          final Uri uri = Identifier.forTrack(Identifier.forCategory(Identifier.CATEGORY_TOP), obj)
              .build();
          visitor.onFile(new TopTrackFile(uri, obj));
        }
      });
    }

    @Override
    public String getPath() {
      return Identifier.CATEGORY_TOP;
    }

    @Override
    public VfsObject resolve(Uri uri, List<String> path) throws IOException {
      final Track track = Identifier.findTopTrack(uri, path);
      return track != null
          ? new TopTrackFile(uri, track)
          : this;
    }

    @Override
    public int compare(VfsObject lh, VfsObject rh) {
      // assume that votes can be compared in lexicographical order
      return -String.CASE_INSENSITIVE_ORDER.compare(((VfsFile) lh).getSize(),
          ((VfsFile) rh).getSize());
    }

    private class TopTrackFile extends BaseTrackFile {

      TopTrackFile(Uri uri, Track module) {
        super(uri, module);
      }

      @Override
      public VfsObject getParent() {
        return TopTracksDir.this;
      }

      @Override
      public String getSize() {
        return module.votes;
      }
    }
  }

  private abstract class BaseTrackFile extends StubObject implements VfsFile {

    protected final Uri uri;
    protected final Track module;

    BaseTrackFile(Uri uri, Track module) {
      this.uri = uri;
      this.module = module;
    }

    @Override
    public Uri getUri() {
      return uri;
    }

    @Override
    public String getName() {
      return uri.getLastPathSegment();
    }

    @Override
    public String getDescription() {
      return module.title;
    }

    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.SHARE_URL.equals(id)) {
        return getShareUrl();
      } else {
        return super.getExtension(id);
      }
    }

    @Override
    public String getSize() {
      return module.duration;
    }

    @Override
    public ByteBuffer getContent() throws IOException {
      return catalog.getTrackContent(module.id);
    }

    private String getShareUrl() {
      return String.format(Locale.US, "http://zxart.ee/zxtune/action%%3aplay/tuneId%%3a%d",
          module.id);
    }
  }
}
