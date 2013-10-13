/**
 * @file
 * @brief
 * @version $Id:$
 * @author
 */
package app.zxtune.fs;

import java.io.IOException;
import java.util.HashSet;
import java.util.List;
import java.util.concurrent.TimeUnit;

import android.content.Context;
import android.net.Uri;
import android.util.Log;
import app.zxtune.R;
import app.zxtune.TimeStamp;
import app.zxtune.fs.zxtunes.Author;
import app.zxtune.fs.zxtunes.Catalog;
import app.zxtune.fs.zxtunes.Catalog.AuthorsVisitor;
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

@VfsDir.Icon(R.drawable.ic_browser_vfs_zxtunes)
final class VfsRootZxtunes implements VfsRoot {

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
  public void enumerate(Visitor visitor) {
    visitor.onDir(allAuthors);
  }

  @Override
  public void find(String mask, Visitor visitor) {
    //TODO
  }

  @Override
  public Uri getUri() {
    return rootUri().build();
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
  public VfsObject resolve(Uri uri) {
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
    return rootUri().appendPath(allAuthors.getName());
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

  private VfsObject resolvePath(Uri uri) {
    final List<String> path = uri.getPathSegments();
    if (path.isEmpty()) {
      return this;
    } else {
      final String category = path.get(POS_CATEGORY);
      if (category.equals(allAuthors.getName())) {
        return resolveAllAuthorsPath(uri, path);
      } else {
        return null;
      }
    }
  }
  
  private VfsObject resolveAllAuthorsPath(Uri uri, List<String> path) {
    if (POS_CATEGORY == path.size() - 1) {
      return allAuthors;
    } else {
      return resolveAuthorPath(uri, path);
    }
  }
  
  private VfsObject resolveAuthorPath(Uri uri, List<String> path) {
    final Author author = resolveAuthor(uri, path.get(POS_AUTHOR_NICK));
    if (author == null) {
      return null;
    }
    final int lastPathElement = path.size() - 1;
    if (lastPathElement == POS_AUTHOR_NICK) {
      return new AuthorDir(author);
    } else if (lastPathElement == POS_AUTHOR_DATE) {
      return resolveAuthorDatePathOrTrack(uri, author, path);
    } else if (lastPathElement == POS_AUTHOR_DATE_TRACK) {
      return resolveAuthorDateTrack(uri, author, path);
    } else {
      return null;
    }
  }
   
  private Author resolveAuthor(Uri uri, String nick) {
    try {
      //nickname is not unique, so use explicit identifier
      final int id = Integer.parseInt(uri.getQueryParameter(PARAM_AUTHOR_ID));
      final FindAuthorVisitor visitor = new FindAuthorVisitor();
      catalog.queryAuthors(visitor, id);
      final Author result = visitor.getResult();
      if (!result.nickname.equals(nick)) {
        Log.d(TAG, String.format("Real author id=%d nick (%s) differs from requested (%s)",
          id, result.nickname, nick));
      }
      return result;
    } catch (Exception e) {//IllegalStateException|NullPointerException|NumberFormatException
      Log.d(TAG, "resolveAuthor(" + uri + ")", e);
    }
    return null;
  }
  
  private static class FindAuthorVisitor implements Catalog.AuthorsVisitor {

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

  private VfsObject resolveAuthorDatePathOrTrack(Uri uri, Author author, List<String> path) {
    final Integer date = resolveAuthorDate(path);
    if (date != null) {
      return new AuthorDateDir(author, date);
    } else {
      final Track track = resolveTrack(uri, author, null, path.get(POS_AUTHOR_TRACK));
      if (track != null) {
        return new TrackFile(uri, track);
      } else {
        Log.d(TAG, "Unknown URI " + uri);
        return null;
      }
    }
  }
  
  private VfsObject resolveAuthorDateTrack(Uri uri, Author author, List<String> path) {
    final Integer date = resolveAuthorDate(path);
    if (date != null) {
      final Track track = resolveTrack(uri, author, date, path.get(POS_AUTHOR_DATE_TRACK));
      if (track != null) {
        return new TrackFile(uri, track);
      }
    }
    Log.d(TAG, "Unknown URI " + uri);
    return null;
  }
  
  private Integer resolveAuthorDate(List<String> path) {
    try {
      final String date = path.get(POS_AUTHOR_DATE);
      return Integer.valueOf(date);
    } catch (NumberFormatException e) {
      return null;
    }
  }
  
  private static class FindTrackVisitor implements Catalog.TracksVisitor {
    
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
  
  private Track resolveTrack(Uri uri, Author author, Integer date, String filename) {
    try {
      //filename is not unique and track can belong to several authors, so use explicit identifier
      //callers doesn't know that author and date is useless now
      final int id = Integer.parseInt(uri.getQueryParameter(PARAM_TRACK_ID));
      final FindTrackVisitor visitor = new FindTrackVisitor();
      catalog.queryTracks(visitor, id, null/*authorId*/);
      final Track result = visitor.getResult();
      if (!result.filename.equals(filename)) {
        Log.d(TAG, String.format("Real track id=%d filename (%s) differs from requested (%s)",
          id, result.filename, filename));
      }
      return result;
    } catch (Exception e) {//IllegalStateException|NullPointerException|NumberFormatException
      Log.d(TAG, "resolveTrack(" + uri + ")", e);
    }
    return null;
  }

  private class AllAuthorsDir implements VfsDir {

    private final String name;

    AllAuthorsDir() {
      this.name = context.getString(R.string.vfs_zxtunes_authors_name);
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
      return context.getString(R.string.vfs_zxtunes_authors_description);
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryAuthors(new AuthorsVisitor() {
        @Override
        public void accept(Author obj) {
          visitor.onDir(new AuthorDir(obj));
        }
      }, null);
    }

    @Override
    public void find(String mask, Visitor visitor) {
      //TODO
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
    public void enumerate(final Visitor visitor) throws IOException {
      final HashSet<Integer> dates = new HashSet<Integer>();
      catalog.queryTracks(new Catalog.TracksVisitor() {
        @Override
        public void accept(Track obj) {
          if (isEmptyDate(obj.date)) {
            visitor.onFile(new TrackFile(trackUri(author, obj).build(), obj));
          } else {
            dates.add(obj.date);
          }
        }
      }, null/*id*/, author.id);
      for (Integer date : dates) {
        visitor.onDir(new AuthorDateDir(author, date));
      }
    }

    @Override
    public void find(String mask, Visitor visitor) {
      //TODO
    }

    private boolean isEmptyDate(Integer date) {
      return date == null || 0 == date;
    }
  }

  private class AuthorDateDir implements VfsDir {

    private final Author author;
    private final Integer date;

    AuthorDateDir(Author author, int date) {
      this.author = author;
      this.date = date;
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
      return "".intern();
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryTracks(new Catalog.TracksVisitor() {
        @Override
        public void accept(Track obj) {
          if (date.equals(obj.date)) {
            visitor.onFile(new TrackFile(trackDateUri(author, date, obj).build(), obj));
          }
        }
      }, null/*id*/, author.id);
    }

    @Override
    public void find(String mask, Visitor visitor) {
      // TODO
    }
  }

  private final static TimeStamp FRAME_DURATION = TimeStamp.createFrom(20, TimeUnit.MILLISECONDS);
  
  private class TrackFile implements VfsFile {

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
    public String getSize() {
      return FRAME_DURATION.multiplies(module.duration).toString();
    }

    @Override
    public byte[] getContent() throws IOException {
      return catalog.getTrackContent(module.id);
    }
  }
}
