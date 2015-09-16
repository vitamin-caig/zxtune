/**
 *
 * @file
 *
 * @brief Implementation of VfsRoot over http://zxtunes.com catalogue
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.net.Uri;
import android.util.SparseIntArray;
import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.TimeStamp;
import app.zxtune.fs.zxtunes.Author;
import app.zxtune.fs.zxtunes.Catalog;
import app.zxtune.fs.zxtunes.Track;

/**
 * 
 * Paths
 * 
 * 1) zxtune:/
 * 2) zxtune:/${authors_folder}
 * 3) zxtune:/${authors_folder}/${author_name}?author=${author_id}
 * 4) zxtune:/${authors_folder}/${author_name}/${date}?author=${author_id}
 * 5) zxtune:/${authors_folder}/${author_name}/${Filename}?author=${author_id}&track=${track_id}
 * 6) zxtune:/${authors_folder}/${author_name}/${date}?author=${author_id}&track=${track_id}
 * 
 * means
 * 
 * 1) root
 * 2) authors root:
 * 3) specific author's root, modules without date, dates folders:
 * 4) author's modules by date:
 * 5) author's module without date:
 * 6) author's module with date
 * 
 * resolving executed sequentally in despite of explicit parameters. E.g.
 * author=${author_id} parameter is not analyzed for cases 1 and 2,
 * track=${track_id} parameter is not analyzed for cases 1, 2, 3 and 4
 */

final class VfsRootZxtunes extends StubObject implements VfsRoot {

  private final static String TAG = VfsRootZxtunes.class.getName();

  private final static String SCHEME = "zxtunes";

  //indices of parts in path
  private final static int POS_CATEGORY = 0;
  private final static int POS_AUTHOR_NICK = 1;
  private final static int POS_AUTHOR_DATE = 2;
  private final static int POS_AUTHOR_TRACK = 2;
  private final static int POS_AUTHOR_DATE_TRACK = 3;

  private final static String PARAM_AUTHOR_ID = "author";
  private final static String PARAM_TRACK_ID = "track";

  private final Context context;
  private final Catalog catalog;
  private final AllAuthorsDir allAuthors;

  public VfsRootZxtunes(Context context) {
    this.context = context;
    this.catalog = Catalog.create(context);
    this.allAuthors = new AllAuthorsDir();
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_zxtunes_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_zxtunes_root_description);
  }

  @Override
  public Uri getUri() {
    return rootUri().build();
  }
  
  @Override
  public VfsObject getParent() {
    return null;
  }

  @Override
  public Object getExtension(String id) {
    if (VfsExtensions.ICON_RESOURCE.equals(id)) {
      return R.drawable.ic_browser_vfs_zxtunes;
    } else {
      return super.getExtension(id);
    }
  }
  
  @Override
  public void enumerate(Visitor visitor) {
    visitor.onDir(allAuthors);
  }
  
  @Override
  public VfsObject resolve(Uri uri) throws IOException {
    if (SCHEME.equals(uri.getScheme())) {
      return resolvePath(uri);
    } else {
      return null;
    }
  }

  private Uri.Builder rootUri() {
    return new Uri.Builder().scheme(SCHEME);
  }

  private Uri.Builder allAuthorsUri() {
    return rootUri().appendPath(AllAuthorsDir.PATH);
  }

  private Uri.Builder authorUri(Author author) {
    return allAuthorsUri()
        .appendPath(author.nickname)
        .appendQueryParameter(PARAM_AUTHOR_ID, Integer.toString(author.id));
  }

  private Uri.Builder authorDateUri(Author author, Integer date) {
    return authorUri(author)
        .appendPath(date.toString());
  }

  private Uri.Builder trackUri(Author author, Track track) {
    return authorUri(author)
        .appendPath(track.filename)
        .appendQueryParameter(PARAM_TRACK_ID, Integer.toString(track.id));
  }

  private Uri.Builder trackDateUri(Author author, Integer date, Track track) {
    return authorDateUri(author, date)
        .appendPath(track.filename)
        .appendQueryParameter(PARAM_TRACK_ID, Integer.toString(track.id));
  }

  private VfsObject resolvePath(Uri uri) throws IOException {
    final List<String> path = uri.getPathSegments();
    if (path.isEmpty()) {
      return this;
    } else {
      final String category = path.get(POS_CATEGORY);
      if (category.equals(AllAuthorsDir.PATH)
        //compatibility
        || category.equals(allAuthors.getName())) {
        return allAuthors.resolve(uri, path);
      } else {
        return null;
      }
    }
  }
  
  
  private static class FindAuthorVisitor extends Catalog.AuthorsVisitor {

    private Author result;

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

  private static class FindTrackVisitor extends Catalog.TracksVisitor {
    
    private Track result;

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
  
  private Track resolveTrack(Uri uri, Author author, Integer date, String filename) throws IOException {
    try {
      //filename is not unique and track can belong to several authors, so use explicit identifier
      //callers doesn't know that author and date is useless now
      final int id = Integer.parseInt(uri.getQueryParameter(PARAM_TRACK_ID));
      final FindTrackVisitor visitor = new FindTrackVisitor();
      catalog.queryAuthorTracks(author, id, visitor);
      final Track result = visitor.getResult();
      if (!result.filename.equals(filename)) {
        Log.d(TAG, "Real track id=%d filename (%s) differs from requested (%s)",
          id, result.filename, filename);
      }
      return result;
    } catch (IOException e) {
      throw e;
    } catch (Exception e) {//IllegalStateException|NullPointerException|NumberFormatException
      Log.d(TAG, e, "resolveTrack %s", uri);
    }
    return null;
  }

  private class AllAuthorsDir extends StubObject implements VfsDir {

    // Static locale-independent path of all authors' dir
    final static String PATH = "Authors";

    @Override
    public Uri getUri() {
      return allAuthorsUri().build();
    }

    @Override
    public String getName() {
      return context.getString(R.string.vfs_zxtunes_authors_name);
    }

    @Override
    public String getDescription() {
      return context.getString(R.string.vfs_zxtunes_authors_description);
    }

    @Override
    public VfsObject getParent() {
      return VfsRootZxtunes.this;
    }
    
    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryAuthors(null/*all*/, new Catalog.AuthorsVisitor() {
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
    
    final VfsObject resolve(Uri uri, List<String> path) throws IOException {
      if (POS_CATEGORY == path.size() - 1) {
        return this;
      } else {
        return resolveAuthorPath(uri, path);
      }
    }

    private VfsObject resolveAuthorPath(Uri uri, List<String> path) throws IOException {
      final Author author = resolveAuthor(uri, path.get(POS_AUTHOR_NICK));
      if (author == null) {
        return null;
      } else {
        return new AuthorDir(author).resolve(uri, path);
      }
    }
     
    private Author resolveAuthor(Uri uri, String nick) throws IOException {
      try {
        //nickname is not unique, so use explicit identifier
        final int id = Integer.parseInt(uri.getQueryParameter(PARAM_AUTHOR_ID));
        final FindAuthorVisitor visitor = new FindAuthorVisitor();
        catalog.queryAuthors(id, visitor);
        final Author result = visitor.getResult();
        if (!result.nickname.equals(nick)) {
          Log.d(TAG, "Real author id=%d nick (%s) differs from requested (%s)",
            id, result.nickname, nick);
        }
        return result;
      } catch (IOException e) {
        throw e;
      } catch (Exception e) {//IllegalStateException|NullPointerException|NumberFormatException
        Log.d(TAG, e, "resolveAuthor %s", uri);
      }
      return null;
    }
  }

  private class AuthorDir extends StubObject implements VfsDir {

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
    public VfsObject getParent() {
      return VfsRootZxtunes.this.allAuthors;
    }
    
    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      final SparseIntArray dates = new SparseIntArray();
      catalog.queryAuthorTracks(author, null/*all*/, new Catalog.TracksVisitor() {
        @Override
        public void setCountHint(int size) {
          visitor.onItemsCount(size);
        }

        @Override
        public void accept(Track obj) {
          if (isEmptyDate(obj.date)) {
            visitor.onFile(new AuthorNodateTrackFile(trackUri(author, obj).build(), obj));
          } else {
            dates.put(obj.date, 1 + dates.get(obj.date));
          }
        }
      });
      for (int i = 0, lim = dates.size(); i != lim; ++i) {
        visitor.onDir(new AuthorDateDir(author, dates.keyAt(i), dates.valueAt(i)));
      }
    }

    private boolean isEmptyDate(Integer date) {
      return date == null || 0 == date;
    }

    private VfsObject resolve(Uri uri, List<String> path) throws IOException {
      final int lastPathElement = path.size() - 1;
      if (lastPathElement == POS_AUTHOR_NICK) {
        return this;
      } else if (lastPathElement >= POS_AUTHOR_DATE) {
        return resolveAuthorDatePathOrTrack(uri, path);
      } else {
        return null;
      }
    }
    
    private VfsObject resolveAuthorDatePathOrTrack(Uri uri, List<String> path) throws IOException {
      final Integer date = resolveAuthorDate(path);
      if (date != null) {
        return new AuthorDateDir(author, date).resolve(uri, path);
      } else {
        final Track track = resolveTrack(uri, author, null, path.get(POS_AUTHOR_TRACK));
        if (track != null) {
          return new AuthorNodateTrackFile(uri, track);
        } else {
          Log.d(TAG, "Unknown URI %s");
          return null;
        }
      }
    }
    
    private Integer resolveAuthorDate(List<String> path) {
      try {
        final String date = path.get(POS_AUTHOR_DATE);
        return Integer.valueOf(date);
      } catch (NumberFormatException e) {
        return null;
      }
    }
    
    private class AuthorNodateTrackFile extends TrackFile {

      AuthorNodateTrackFile(Uri uri, Track module) {
        super(uri, module);
      }
      
      @Override
      public VfsObject getParent() {
        return AuthorDir.this;
      }
    }
  }

  private class AuthorDateDir extends StubObject implements VfsDir {

    private final Author author;
    private final Integer date;
    private final int count;

    AuthorDateDir(Author author, Integer date, int count) {
      this.author = author;
      this.date = date;
      this.count = count;
    }

    AuthorDateDir(Author author, Integer date) {
      this(author, date, 0);
    }
    
    @Override
    public Uri getUri() {
      return authorDateUri(author, date).build();
    }

    @Override
    public String getName() {
      return Integer.toString(date);
    }

    @Override
    public String getDescription() {
      return context.getResources().getQuantityString(R.plurals.tracks, count, count);
    }
    
    @Override
    public VfsObject getParent() {
      return new AuthorDir(author);
    }    

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryAuthorTracks(author, null/*all*/, new Catalog.TracksVisitor() {
        @Override
        public void setCountHint(int size) {
          visitor.onItemsCount(size);
        }

        @Override
        public void accept(Track obj) {
          if (date.equals(obj.date)) {
            visitor.onFile(new AuthorDatedTrackFile(trackDateUri(author, date, obj).build(), obj));
          }
        }
      });
    }
    
    private VfsObject resolve(Uri uri, List<String> path) throws IOException {
      if (path.size() - 1 == POS_AUTHOR_DATE) {
        return this;
      } else {
        final Track track = resolveTrack(uri, author, date, path.get(POS_AUTHOR_DATE_TRACK));
        if (track != null) {
          return new AuthorDatedTrackFile(uri, track);
        } else {
          Log.d(TAG, "Unknown URI %s", uri);
          return null;
        }
      }
    }
    
    private class AuthorDatedTrackFile extends TrackFile {

      AuthorDatedTrackFile(Uri uri, Track module) {
        super(uri, module);
      }
      
      @Override
      public VfsObject getParent() {
        return AuthorDateDir.this;
      }
    }
  }

  private final static TimeStamp FRAME_DURATION = TimeStamp.createFrom(20, TimeUnit.MILLISECONDS);
  
  private abstract class TrackFile extends StubObject implements VfsFile {

    private final Uri uri;
    private final Track module;

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
    public Object getExtension(String id) {
      if (VfsExtensions.SHARE_URL.equals(id)) {
        return getShareUrl();
      } else {
        return super.getExtension(id);
      }
    }

    @Override
    public String getSize() {
      return module.duration != null
        ? FRAME_DURATION.multiplies(module.duration).toString()
        : "".intern();
    }

    @Override
    public ByteBuffer getContent() throws IOException {
      return catalog.getTrackContent(module.id);
    }
    
    private String getShareUrl() {
      final int authorId = Integer.parseInt(uri.getQueryParameter(PARAM_AUTHOR_ID));
      return String.format(Locale.US, "http://zxtunes.com/author.php?id=%d&play=%d", authorId, module.id);
    }
  }
}
