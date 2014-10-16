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
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import android.util.SparseIntArray;
import app.zxtune.R;
import app.zxtune.fs.zxart.Author;
import app.zxtune.fs.zxart.Catalog;
import app.zxtune.fs.zxart.Party;
import app.zxtune.fs.zxart.Track;
import app.zxtune.ui.IconSource;

public class VfsRootZxart implements VfsRoot, IconSource {

  private final static String TAG = VfsRootZxart.class.getName();

  private final static String SCHEME = "zxart";

  // indices of parts in path
  private final static int POS_CATEGORY = 0;
  private final static int POS_AUTHOR_NICK = 1;
  private final static int POS_AUTHOR_YEAR = 2;
  private final static int POS_AUTHOR_YEAR_TRACK = 3;
  private final static int POS_PARTY_YEAR = 1;
  private final static int POS_PARTY_NAME = 2;
  private final static int POS_PARTY_COMPO = 3;
  private final static int POS_PARTY_TRACK = 4;
  private final static int POS_TOP_TRACK = 1;

  private final static String PARAM_AUTHOR_ID = "author";
  private final static String PARAM_PARTY_ID = "party";
  private final static String PARAM_TRACK_ID = "track";

  private final Context context;
  private final Catalog catalog;
  private final GroupsDir groups[];

  public VfsRootZxart(Context context) {
    this.context = context;
    this.catalog = Catalog.create(context);
    this.groups = new GroupsDir[] {
        new AllAuthorsDir(),
        new AllPartiesDir(),
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
    return rootUri().build();
  }

  @Override
  public VfsDir getParent() {
    return null;
  }

  @Override
  public void enumerate(Visitor visitor) {
    for (GroupsDir group : groups) {
      visitor.onDir(group);
    }
  }

  @Override
  public void find(String mask, Visitor visitor) {
    // TODO
  }

  @Override
  public VfsObject resolve(Uri uri) {
    if (SCHEME.equals(uri.getScheme())) {
      return resolvePath(uri);
    } else {
      return null;
    }
  }

  @Override
  public int getResourceId() {
    return R.drawable.ic_browser_vfs_zxart;
  }

  private static Uri.Builder rootUri() {
    return new Uri.Builder().scheme(SCHEME);
  }

  private static Uri.Builder allAuthorsUri() {
    return rootUri().appendPath(AllAuthorsDir.PATH);
  }

  private static Uri.Builder authorUri(Author author) {
    return allAuthorsUri()
        .appendPath(author.nickname)
        .appendQueryParameter(PARAM_AUTHOR_ID, Integer.toString(author.id));
  }

  private static Uri.Builder authorYearUri(Author author, int year) {
    return authorUri(author)
        .appendPath(year != 0 ? Integer.toString(year) : AuthorYearDir.UNKNOWNYEAR_PATH);
  }

  private static Uri.Builder trackUri(Author author, int year, Track track) {
    return authorYearUri(author, year)
        .appendPath(track.filename)
        .appendQueryParameter(PARAM_TRACK_ID, Integer.toString(track.id));
  }
  
  private static Uri.Builder allPartiesUri() {
    return rootUri().appendPath(AllPartiesDir.PATH);
  }
  
  private static Uri.Builder partyYearUri(int year) {
    return allPartiesUri().appendPath(Integer.toString(year));
  }
  
  private static Uri.Builder partyUri(Party party) {
    return partyYearUri(party.year)
        .appendPath(party.name)
        .appendQueryParameter(PARAM_PARTY_ID, Integer.toString(party.id));
  }
  
  private static Uri.Builder partyCompoUri(Party party, String compo) {
    return partyUri(party)
        .appendPath(compo);
  }
  
  private static Uri.Builder trackUri(Party party, String compo, Track track) {
    return partyCompoUri(party, compo)
        .appendPath(track.filename)
        .appendQueryParameter(PARAM_TRACK_ID, Integer.toString(track.id));
  }
  
  private static Uri.Builder topTracksUri() {
    return rootUri().appendPath(TopTracksDir.PATH);
  }
  
  private static Uri.Builder topTrackUri(Track track) {
    return topTracksUri()
        .appendPath(track.filename)
        .appendQueryParameter(PARAM_TRACK_ID, Integer.toString(track.id));
  }

  private VfsObject resolvePath(Uri uri) {
    final List<String> path = uri.getPathSegments();
    if (path.isEmpty()) {
      return this;
    } else {
      final String category = path.get(POS_CATEGORY);
      for (GroupsDir group : groups) {
        if (category.equals(group.getPath())) {
          return group.resolve(uri, path);
        }
      }
      return null;
    }
  }

  private interface GroupsDir extends VfsDir {
    String getPath();

    VfsObject resolve(Uri uri, List<String> path);
  }

  private static class FindAuthorVisitor implements Catalog.AuthorsVisitor {

    private Author result;

    @Override
    public void setCountHint(int size) {
    }

    @Override
    public void accept(Author obj) {
      if (result == null) {
        result = obj;
      } else {
        throw new IllegalStateException();
      }
    }

    final Author getResult() {
      return result;
    }
  }

  private Author resolveAuthor(Uri uri, String nick) {
    try {
      // nickname is not unique, so use explicit identifier
      final int id = Integer.parseInt(uri.getQueryParameter(PARAM_AUTHOR_ID));
      final FindAuthorVisitor visitor = new FindAuthorVisitor();
      catalog.queryAuthors(visitor, id);
      final Author result = visitor.getResult();
      if (!result.nickname.equals(nick)) {
        Log.d(TAG, String.format("Real author id=%d nick (%s) differs from requested (%s)",
            id, result.nickname, nick));
      }
      return result;
    } catch (Exception e) {// IllegalStateException|NullPointerException|NumberFormatException
      Log.d(TAG, "resolveAuthor(" + uri + ")", e);
    }
    return null;
  }

  private static class FindPartyVisitor implements Catalog.PartiesVisitor {

    private Party result;

    @Override
    public void setCountHint(int size) {
    }

    @Override
    public void accept(Party obj) {
      if (result == null) {
        result = obj;
      } else {
        throw new IllegalStateException();
      }
    }

    final Party getResult() {
      return result;
    }
  }

  private Party resolveParty(Uri uri, String name) {
    try {
      // name is not unique, so use explicit identifier
      final int id = Integer.parseInt(uri.getQueryParameter(PARAM_PARTY_ID));
      final FindPartyVisitor visitor = new FindPartyVisitor();
      catalog.queryParties(visitor, id);
      final Party result = visitor.getResult();
      if (!result.name.equals(name)) {
        Log.d(TAG, String.format("Real party id=%d name (%s) differs from requested (%s)",
            id, result.name, name));
      }
      return result;
    } catch (Exception e) {// IllegalStateException|NullPointerException|NumberFormatException
      Log.d(TAG, "resolveParty(" + uri + ")", e);
    }
    return null;
  }
  
  private static class FindTrackVisitor implements Catalog.TracksVisitor {

    private Track result;

    @Override
    public void setCountHint(int size) {
    }

    @Override
    public void accept(Track obj) {
      if (result == null) {
        result = obj;
      } else {
        throw new IllegalStateException();
      }
    }

    final Track getResult() {
      return result;
    }
  }

  private Track resolveTrack(Uri uri, Author author, int year, String filename) {
    try {
      // filename is not unique and track can belong to several authors, so use
      // explicit identifier
      // callers doesn't know that author and year is useless now
      final int id = Integer.parseInt(uri.getQueryParameter(PARAM_TRACK_ID));
      final FindTrackVisitor visitor = new FindTrackVisitor();
      catalog.queryAuthorTracks(visitor, id, author.id);
      final Track result = visitor.getResult();
      if (!result.filename.equals(filename)) {
        Log.d(TAG, String.format("Real track id=%d filename (%s) differs from requested (%s)",
            id, result.filename, filename));
      }
      return result;
    } catch (Exception e) {// IllegalStateException|NullPointerException|NumberFormatException
      Log.d(TAG, "resolveTrack(" + uri + ")", e);
    }
    return null;
  }

  private Track resolveTrack(Uri uri, Party party, String filename) {
    try {
      // filename is not unique and track can belong to several parties(?), so use
      // explicit identifier
      // callers doesn't know that party is useless
      final int id = Integer.parseInt(uri.getQueryParameter(PARAM_TRACK_ID));
      final FindTrackVisitor visitor = new FindTrackVisitor();
      catalog.queryPartyTracks(visitor, id, party.id);
      final Track result = visitor.getResult();
      if (!result.filename.equals(filename)) {
        Log.d(TAG, String.format("Real track id=%d filename (%s) differs from requested (%s)",
            id, result.filename, filename));
      }
      return result;
    } catch (Exception e) {// IllegalStateException|NullPointerException|NumberFormatException
      Log.d(TAG, "resolveTrack(" + uri + ")", e);
    }
    return null;
  }

  private Track resolveTrack(Uri uri, String filename) {
    try {
      final int id = Integer.parseInt(uri.getQueryParameter(PARAM_TRACK_ID));
      final FindTrackVisitor visitor = new FindTrackVisitor();
      catalog.queryTopTracks(visitor, id, 1);
      final Track result = visitor.getResult();
      if (!result.filename.equals(filename)) {
        Log.d(TAG, String.format("Real track id=%d filename (%s) differs from requested (%s)",
            id, result.filename, filename));
      }
      return result;
    } catch (Exception e) {// IllegalStateException|NullPointerException|NumberFormatException
      Log.d(TAG, "resolveTrack(" + uri + ")", e);
    }
    return null;
  }
  
  private class AllAuthorsDir implements GroupsDir {

    // Static locale-independent path of all authors' dir
    final static String PATH = "Authors";
    private final String name;
    private final String desc;

    AllAuthorsDir() {
      this.name = context.getString(R.string.vfs_zxart_authors_name);
      this.desc = context.getString(R.string.vfs_zxart_authors_description);
    }

    @Override
    public Uri getUri() {
      return allAuthorsUri().build();
    }

    @Override
    public String getName() {
      return name;
    }

    @Override
    public String getDescription() {
      return desc;
    }

    @Override
    public VfsDir getParent() {
      return VfsRootZxart.this;
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
      }, null);
    }

    @Override
    public void find(String mask, Visitor visitor) {
      // TODO
    }

    @Override
    public String getPath() {
      return PATH;
    }

    @Override
    public VfsObject resolve(Uri uri, List<String> path) {
      final int lastPathElement = path.size() - 1;
      if (lastPathElement == POS_CATEGORY) {
        return this;
      }
      final Author author = resolveAuthor(uri, path.get(POS_AUTHOR_NICK));
      if (author == null) {
        return null;
      }
      if (lastPathElement == POS_AUTHOR_NICK) {
        return new AuthorDir(author);
      } else if (lastPathElement == POS_AUTHOR_YEAR) {
        return resolveAuthorYearPath(uri, author, path);
      } else if (lastPathElement == POS_AUTHOR_YEAR_TRACK) {
        return resolveAuthorYearTrack(uri, author, path);
      } else {
        return null;
      }
    }

    private VfsObject resolveAuthorYearPath(Uri uri, Author author, List<String> path) {
      final int year = resolveAuthorYear(path);
      return new AuthorYearDir(author, year);
    }

    private VfsObject resolveAuthorYearTrack(Uri uri, Author author, List<String> path) {
      final int year = resolveAuthorYear(path);
      final Track track = resolveTrack(uri, author, year, path.get(POS_AUTHOR_YEAR_TRACK));
      if (track != null) {
        return new TrackFile(uri, track);
      } else {
        Log.d(TAG, "Unknown URI " + uri);
        return null;
      }
    }

    private int resolveAuthorYear(List<String> path) {
      try {
        final String year = path.get(POS_AUTHOR_YEAR);
        return AuthorYearDir.UNKNOWNYEAR_PATH.equals(year) ? 0 : Integer.valueOf(year);
      } catch (NumberFormatException e) {
        return 0;
      }
    }
  }

  private class AuthorDir implements VfsDir {

    private final Author author;

    AuthorDir(Author author) {
      this.author = author;
    }

    @Override
    public Uri getUri() {
      return authorUri(author).build();
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
    public VfsDir getParent() {
      return groups[0];// TODO
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      final SparseIntArray years = new SparseIntArray();
      
      catalog.queryAuthorTracks(new Catalog.TracksVisitor() {
        @Override
        public void setCountHint(int size) {
        }

        @Override
        public void accept(Track obj) {
          years.put(obj.year, 1 + years.get(obj.year));
        }
      }, null/* id */, author.id);
      visitor.onItemsCount(years.size());
      for (int i = 0, lim = years.size(); i != lim; ++i) {
        //handle count when required
        visitor.onDir(new AuthorYearDir(author, years.keyAt(i)));
      }
    }

    @Override
    public void find(String mask, Visitor visitor) {
      // TODO
    }
  }

  private class AuthorYearDir implements VfsDir {

    static final String UNKNOWNYEAR_PATH = "unknown";
    private final Author author;
    private final int year;

    AuthorYearDir(Author author, int year) {
      this.author = author;
      this.year = year;
    }

    @Override
    public Uri getUri() {
      return authorYearUri(author, year).build();
    }

    @Override
    public String getName() {
      return year != 0
          ? Integer.toString(year)
          : context.getString(R.string.vfs_zxart_unknown_year_name);
    }

    @Override
    public String getDescription() {
      return "";
    }

    @Override
    public VfsDir getParent() {
      return new AuthorDir(author);
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryAuthorTracks(new Catalog.TracksVisitor() {
        @Override
        public void setCountHint(int size) {
          visitor.onItemsCount(size);
        }

        @Override
        public void accept(Track obj) {
          if (year == obj.year) {
            visitor.onFile(new TrackFile(trackUri(author, year, obj).build(), obj));
          }
        }
      }, null/* id */, author.id);
    }

    @Override
    public void find(String mask, Visitor visitor) {
      // TODO
    }
  }

  private class AllPartiesDir implements GroupsDir {

    // Static locale-independent path of all parties' dir
    final static String PATH = "Parties";
    private final String name;
    private final String desc;

    AllPartiesDir() {
      this.name = context.getString(R.string.vfs_zxart_parties_name);
      this.desc = context.getString(R.string.vfs_zxart_parties_description);
    }

    @Override
    public Uri getUri() {
      return allPartiesUri().build();
    }

    @Override
    public String getName() {
      return name;
    }

    @Override
    public String getDescription() {
      return desc;
    }

    @Override
    public VfsDir getParent() {
      return VfsRootZxart.this;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      final SparseIntArray years = new SparseIntArray();
      catalog.queryParties(new Catalog.PartiesVisitor() {
        @Override
        public void setCountHint(int hint) {
        }

        @Override
        public void accept(Party obj) {
          years.put(obj.year, 1 + years.get(obj.year));
        }
      }, null/*id*/);
      visitor.onItemsCount(years.size());
      for (int i = 0, lim = years.size(); i != lim; ++i) {
        visitor.onDir(new PartyYearDir(years.keyAt(i), years.valueAt(i))); 
      }
    }

    @Override
    public void find(String mask, Visitor visitor) {
      // TODO
    }

    @Override
    public String getPath() {
      return PATH;
    }

    @Override
    public VfsObject resolve(Uri uri, List<String> path) {
      final int lastPathElement = path.size() - 1;
      if (lastPathElement == POS_CATEGORY) {
        return this;
      } else if (lastPathElement == POS_PARTY_YEAR) {
        return resolvePartyYearDir(path);
      }
      final Party party = resolveParty(uri, path.get(POS_PARTY_NAME));
      if (party == null) {
        return null;
      }
      if (lastPathElement == POS_PARTY_NAME) {
        return new PartyDir(party);
      } else if (lastPathElement == POS_PARTY_COMPO) {
        return resolvePartyCompoDir(uri, party, path);
      } else if (lastPathElement == POS_PARTY_TRACK) {
        return resolvePartyTrack(uri, party, path.get(POS_PARTY_TRACK));
      } else {
        return null;
      }
    }
    
    private VfsObject resolvePartyYearDir(List<String> path) {
      final int year = resolvePartyYear(path);
      return new PartyYearDir(year);
    }
    
    private VfsObject resolvePartyCompoDir(Uri uri, Party party, List<String> path) {
      final String compo = path.get(POS_PARTY_COMPO);
      //compatibility
      if (uri.getQueryParameter(PARAM_TRACK_ID) != null) {
        return resolvePartyTrack(uri, party, compo);
      } else {
        return new PartyCompoDir(party, compo);
      }
    }
    
    private VfsObject resolvePartyTrack(Uri uri, Party party, String filename) {
      final Track track = resolveTrack(uri, party, filename);
      if (track != null) {
        return new TrackFile(uri, track);
      } else {
        Log.d(TAG, "Unknown URI " + uri);
        return null;
      }
    }
    
    private int resolvePartyYear(List<String> path) {
      try {
        final String year = path.get(POS_PARTY_YEAR);
        return Integer.valueOf(year);
      } catch (NumberFormatException e) {
        return 0;
      }
    }
  }
  
  private class PartyYearDir implements VfsDir {
    
    private final int year;
    private final int count;
    
    PartyYearDir(int year, int count) {
      this.year = year;
      this.count = count;
    }
    
    PartyYearDir(int year) {
      this(year, 0);
    }

    @Override
    public Uri getUri() {
      return partyYearUri(year).build();
    }

    @Override
    public String getName() {
      return Integer.toString(year);
    }

    @Override
    public String getDescription() {
      return "";
    }

    @Override
    public VfsDir getParent() {
      return VfsRootZxart.this.groups[1];
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      
      catalog.queryParties(new Catalog.PartiesVisitor() {
        
        @Override
        public void setCountHint(int size) {
        }
        
        @Override
        public void accept(Party obj) {
          if (obj.year == year) {
            visitor.onDir(new PartyDir(obj));
          }
        }
      }, null/*id*/);
    }

    @Override
    public void find(String mask, Visitor visitor) {
      // TODO Auto-generated method stub
    }
  }
  
  private class PartyDir implements VfsDir {
    
    private final Party party;
    
    PartyDir(Party party) {
      this.party = party;
    }

    @Override
    public Uri getUri() {
      return partyUri(party).build();
    }

    @Override
    public String getName() {
      return party.name;
    }

    @Override
    public String getDescription() {
      return "";
    }

    @Override
    public VfsDir getParent() {
      return new PartyYearDir(party.year);
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      final HashSet<String> compos = new HashSet<String>();
      catalog.queryPartyTracks(new Catalog.TracksVisitor() {
        
        @Override
        public void setCountHint(int size) {
        }
        
        @Override
        public void accept(Track obj) {
          compos.add(obj.compo);
        }
      }, null/*id*/, party.id);
      visitor.onItemsCount(compos.size());
      for (String compo : compos) {
        visitor.onDir(new PartyCompoDir(party, compo)); 
      }
    }

    @Override
    public void find(String mask, Visitor visitor) {
      // TODO Auto-generated method stub
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
  
  //custom ordering by partyplace
  private class PartyCompoDir implements VfsDir, Comparator<VfsObject> {
    
    private final Party party;
    private final CompoIdentifier compo;

    PartyCompoDir(Party party, String compo) {
      this.party = party;
      this.compo = CompoIdentifier.getId(compo);
    }

    @Override
    public Uri getUri() {
      return partyCompoUri(party, compo.name()).build();
    }

    @Override
    public String getName() {
      return context.getString(compo.getResource());
    }

    @Override
    public String getDescription() {
      return "";
    }

    @Override
    public VfsDir getParent() {
      return new PartyDir(party);
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryPartyTracks(new Catalog.TracksVisitor() {
        @Override
        public void setCountHint(int size) {
          visitor.onItemsCount(size);
        }

        @Override
        public void accept(Track obj) {
          if (compo.name().equals(obj.compo)) {
            visitor.onFile(new PartyplacedTrackFile(trackUri(party, compo.name(), obj).build(), obj));
          }
        }
      }, null/* id */, party.id);
    }

    @Override
    public void find(String mask, Visitor visitor) {
      // TODO
    }

    @Override
    public int compare(VfsObject lh, VfsObject rh) {
      final int lhPlace = ((TrackFile)lh).module.partyplace;
      final int rhPlace = ((TrackFile)rh).module.partyplace;
      return lhPlace == rhPlace
        ? 0
        : (lhPlace < rhPlace ? -1 : +1);
    }
  }
  
  //custom ordering by votes desc
  class TopTracksDir implements GroupsDir, Comparator<VfsObject> {

    // Static locale-independent path of top tracks' dir
    final static String PATH = "Top";
    private final String name;
    private final String desc;

    TopTracksDir() {
      this.name = context.getString(R.string.vfs_zxart_toptracks_name);
      this.desc = context.getString(R.string.vfs_zxart_toptracks_description);
    }

    @Override
    public Uri getUri() {
      return topTracksUri().build();
    }

    @Override
    public String getName() {
      return name;
    }

    @Override
    public String getDescription() {
      return desc;
    }
    
    @Override
    public VfsDir getParent() {
      return VfsRootZxart.this;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryTopTracks(new Catalog.TracksVisitor() {
        
        @Override
        public void setCountHint(int size) {
          visitor.onItemsCount(size);
        }
        
        @Override
        public void accept(Track obj) {
          visitor.onFile(new VotedTrackFile(topTrackUri(obj).build(), obj));
        }
      }, null/*id*/, 100);
    }

    @Override
    public void find(String mask, Visitor visitor) {
      // TODO Auto-generated method stub
    }

    @Override
    public String getPath() {
      return PATH;
    }

    @Override
    public VfsObject resolve(Uri uri, List<String> path) {
      final int lastPathElement = path.size() - 1;
      if (lastPathElement == POS_CATEGORY) {
        return this;
      } else if (lastPathElement == POS_TOP_TRACK) {
        return resolveTopTrack(uri, path);
      } else {
        return null;
      }
    }
    
    private VfsObject resolveTopTrack(Uri uri, List<String> path) {
      final Track track = resolveTrack(uri, path.get(POS_TOP_TRACK));
      if (track != null) {
        return new TrackFile(uri, track);
      } else {
        Log.d(TAG, "Unknown URI " + uri);
        return null;
      }
    }

    @Override
    public int compare(VfsObject lh, VfsObject rh) {
      //assume that votes can be compared in lexicographical order
      return -String.CASE_INSENSITIVE_ORDER.compare(((VfsFile)lh).getSize(), ((VfsFile)rh).getSize());
    }
  }
  
  private class TrackFile implements VfsFile {

    private final Uri uri;
    protected final Track module;

    TrackFile(Uri uri, Track module) {
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
    public String getSize() {
      return module.duration;
    }

    @Override
    public ByteBuffer getContent() throws IOException {
      return catalog.getTrackContent(module.id);
    }
  }

  private class VotedTrackFile extends TrackFile {

    VotedTrackFile(Uri uri, Track module) {
      super(uri, module);
    }

    @Override
    public String getSize() {
      return module.votes;
    }
  }
  
  private class PartyplacedTrackFile extends TrackFile {
    
    PartyplacedTrackFile(Uri uri, Track module) {
      super(uri, module);
    }
    
    @Override
    public String getSize() {
      return Integer.toString(module.partyplace);
    }
  }
}
