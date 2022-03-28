/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.content.res.Resources;
import android.net.Uri;
import android.util.SparseIntArray;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.Comparator;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;

import app.zxtune.R;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.fs.zxart.Author;
import app.zxtune.fs.zxart.CachingCatalog;
import app.zxtune.fs.zxart.Catalog;
import app.zxtune.fs.zxart.Identifier;
import app.zxtune.fs.zxart.Party;
import app.zxtune.fs.zxart.RemoteCatalog;
import app.zxtune.fs.zxart.Track;

public class VfsRootZxart extends StubObject implements VfsRoot {

  private static final String TAG = VfsRootZxart.class.getName();

  private final VfsObject parent;
  private final Context context;
  private final CachingCatalog catalog;
  private final GroupingDir[] groups;

  public VfsRootZxart(VfsObject parent, Context context, MultisourceHttpProvider http) {
    this.parent = parent;
    this.context = context;
    this.catalog = Catalog.create(context, http);
    this.groups = new GroupingDir[]{
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
  @Nullable
  public VfsObject getParent() {
    return parent;
  }

  @Override
  public Object getExtension(String id) {
    if (VfsExtensions.SEARCH_ENGINE.equals(id)) {
      //assume search by authors from root
      return new AuthorsSearchEngine();
    } else if (VfsExtensions.ICON.equals(id)) {
      return R.drawable.ic_browser_vfs_zxart;
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
  @Nullable
  public VfsObject resolve(Uri uri) {
    if (Identifier.isFromRoot(uri)) {
      final List<String> path = uri.getPathSegments();
      return resolve(uri, path);
    } else {
      return null;
    }
  }

  @Nullable
  private VfsObject resolve(Uri uri, List<String> path) {
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

  private abstract static class GroupingDir extends StubObject implements VfsDir {

    @Override
    public Uri getUri() {
      return Identifier.forCategory(getPath()).build();
    }

    abstract String getPath();

    @Nullable
    abstract VfsObject resolve(Uri uri, List<String> path);
  }

  private class AuthorsDir extends GroupingDir {

    @Override
    public String getName() {
      return context.getString(R.string.vfs_zxart_authors_name);
    }

    @Override
    public VfsObject getParent() {
      return VfsRootZxart.this;
    }

    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.SEARCH_ENGINE.equals(id)) {
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
    public VfsObject resolve(Uri uri, List<String> path) {
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

  @Nullable
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
      return author.getNickname();
    }

    @Override
    public String getDescription() {
      return author.getName();
    }

    @Override
    public VfsObject getParent() {
      return groups[0];// TODO
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      final SparseIntArray years = new SparseIntArray();

      catalog.queryAuthorTracks(author, new Catalog.TracksVisitor() {
        // TODO: remove
        @Override
        public void setCountHint(int size) {}

        @Override
        public void accept(Track obj) {
          years.put(obj.getYear(), 1 + years.get(obj.getYear()));
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
          if (year == obj.getYear()) {
            final Uri uri = Identifier.forTrack(Identifier.forAuthor(author, year), obj).build();
            visitor.onFile(new AuthorTrackFile(uri, obj));
          }
        }
      });
    }
  }

  private class AuthorTrackFile extends BaseTrackFile {

    AuthorTrackFile(Uri uri, Track track) {
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

        // TODO: remove
        @Override
        public void setCountHint(int size) {}

        @Override
        public void accept(Author author, Track track) {
          final Uri uri = Identifier.forTrack(Identifier.forAuthor(author, track.getYear()), track).build();
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
    public VfsObject getParent() {
      return VfsRootZxart.this;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      final SparseIntArray years = new SparseIntArray();
      catalog.queryParties(new Catalog.PartiesVisitor() {
        // TODO: remove
        @Override
        public void setCountHint(int size) {}

        @Override
        public void accept(Party obj) {
          years.put(obj.getYear(), 1 + years.get(obj.getYear()));
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
    public VfsObject resolve(Uri uri, List<String> path) {
      final Track track = Identifier.findPartyTrack(uri, path);
      if (track != null) {
        return new PartyTrackFile(uri, track);
      }
      final VfsObject dir = resolvePartyDir(uri, path);
      return dir != null ? dir : this;
    }
  }

  @Nullable
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
        // TODO: remove
        @Override
        public void setCountHint(int size) {}

        @Override
        public void accept(Party obj) {
          if (obj.getYear() == year) {
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
      return party.getName();
    }

    @Override
    public VfsObject getParent() {
      return new PartyYearDir(party.getYear());
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      final HashSet<String> compos = new HashSet<>();
      catalog.queryPartyTracks(party, new Catalog.TracksVisitor() {
        // TODO: remove
        @Override
        public void setCountHint(int size) {}

        @Override
        public void accept(Track obj) {
          compos.add(obj.getCompo());
        }
      });
      visitor.onItemsCount(compos.size());
      for (String compo : compos) {
        visitor.onDir(new PartyCompoDir(party, compo));
      }
    }
  }

  static class CompoName {
    private final String id;
    private final String name;

    private CompoName(String id, Context ctx) {
      this.id = id;
      this.name = getLocalizedName(ctx);
    }

    final String getId() {
      return id;
    }

    final String getName() {
      return name;
    }

    private String getLocalizedName(Context ctx) {
      final Resources res = ctx.getResources();
      final int resId = res.getIdentifier("vfs_zxart_compo_" + id, "string", ctx.getPackageName());
      return resId != 0
          ? res.getString(resId)
          : id;
    }
  }

  private static class PartyCompoTracksComparator implements Comparator<VfsObject> {
    @Override
    public int compare(VfsObject lh, VfsObject rh) {
      final int lhPlace = ((BaseTrackFile) lh).module.getPartyplace();
      final int rhPlace = ((BaseTrackFile) rh).module.getPartyplace();
      return Integer.compare(lhPlace, rhPlace);
    }

    static Comparator<VfsObject> instance() {
      return Holder.INSTANCE;
    }

    //onDemand holder idiom
    private static class Holder {
      static final PartyCompoTracksComparator INSTANCE = new PartyCompoTracksComparator();
    }
  }

  // custom ordering by party place
  private class PartyCompoDir extends StubObject implements VfsDir {

    private final Party party;
    private final CompoName compo;

    PartyCompoDir(Party party, String compo) {
      this.party = party;
      this.compo = new CompoName(compo, context);
    }

    @Override
    public Uri getUri() {
      return Identifier.forPartyCompo(party, compo.getId()).build();
    }

    @Override
    public String getName() {
      return compo.getName();
    }

    @Override
    public VfsObject getParent() {
      return new PartyDir(party);
    }

    @Nullable
    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.COMPARATOR.equals(id)) {
        return PartyCompoTracksComparator.instance();
      } else {
        return super.getExtension(id);
      }
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
          if (compo.getId().equals(obj.getCompo())) {
            final Uri uri = Identifier.forTrack(Identifier.forPartyCompo(party, compo.getId()), obj)
                .build();
            visitor.onFile(new PartyTrackFile(uri, obj));
          }
        }
      });
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
      return Integer.toString(module.getPartyplace());
    }
  }

  private static class TopTracksComparator implements Comparator<VfsObject> {
    @Override
    public int compare(VfsObject lh, VfsObject rh) {
      // assume that votes can be compared in lexicographical order
      return -String.CASE_INSENSITIVE_ORDER.compare(((VfsFile) lh).getSize(),
          ((VfsFile) rh).getSize());
    }

    static Comparator<VfsObject> instance() {
      return Holder.INSTANCE;
    }

    //onDemand holder idiom
    private static class Holder {
      static final TopTracksComparator INSTANCE = new TopTracksComparator();
    }
  }

  // custom ordering by votes desc
  private class TopTracksDir extends GroupingDir {

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

    @Nullable
    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.COMPARATOR.equals(id)) {
        return TopTracksComparator.instance();
      } else {
        return super.getExtension(id);
      }
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
    public VfsObject resolve(Uri uri, List<String> path) {
      final Track track = Identifier.findTopTrack(uri, path);
      return track != null
          ? new TopTrackFile(uri, track)
          : this;
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
        return module.getVotes();
      }
    }
  }

  private abstract static class BaseTrackFile extends StubObject implements VfsFile {

    final Uri uri;
    final Track module;

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
      return module.getTitle();
    }

    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.CACHE_PATH.equals(id)) {
        return Integer.toString(module.getId());
      } else if (VfsExtensions.DOWNLOAD_URIS.equals(id)) {
        return RemoteCatalog.getTrackUris(module.getId());
      } else if (VfsExtensions.SHARE_URL.equals(id)) {
        return getShareUrl();
      } else {
        return super.getExtension(id);
      }
    }

    @Override
    public String getSize() {
      return module.getDuration();
    }

    private String getShareUrl() {
      return String.format(Locale.US, "https://zxart.ee/zxtune/action%%3aplay/tuneId%%3a%d",
          module.getId());
    }
  }
}
