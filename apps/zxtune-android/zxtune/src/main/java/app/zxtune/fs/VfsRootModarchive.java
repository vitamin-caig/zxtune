/**
 *
 * @file
 *
 * @brief Implementation of VfsRoot over http://amp.dascene.net catalogue
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Locale;

import android.content.Context;
import android.net.Uri;
import android.text.format.Formatter;
import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.fs.modarchive.Author;
import app.zxtune.fs.modarchive.Catalog;
import app.zxtune.fs.modarchive.Genre;
import app.zxtune.fs.modarchive.Identifier;
import app.zxtune.fs.modarchive.Track;

final class VfsRootModarchive extends StubObject implements VfsRoot {

  private final static String TAG = VfsRootModarchive.class.getName();

  private final Context context;
  private final Catalog catalog;
  private final GroupingDir groupings[];

  VfsRootModarchive(Context context, HttpProvider http, VfsCache cache) {
    this.context = context;
    this.catalog = Catalog.create(context, http, cache);
    this.groupings = new GroupingDir[] {
        new AuthorsDir(),
        new GenresDir(),
    };
  }

  @Override
  public Uri getUri() {
    return Identifier.forRoot().build();
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_modarchive_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_modarchive_root_description);
  }

  @Override
  public VfsObject getParent() {
    return null;
  }
  
  @Override
  public Object getExtension(String id) {
    if (VfsExtensions.ICON_RESOURCE.equals(id)) {
      return R.drawable.ic_browser_vfs_modarchive;
    } else if (VfsExtensions.SEARCH_ENGINE.equals(id) && catalog.searchSupported()) {
      //assume root will search by authors
      return new AuthorsSearchEngine();
    } else {
      return super.getExtension(id);
    }
  }

  @Override
  public void enumerate(Visitor visitor) throws IOException {
    for (GroupingDir group : groupings) {
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

  private VfsObject resolve(Uri uri, List<String> path) {
    // due to identical structure of groupings, may resolve here
    // use plain algo with most frequent cases check first
    final Track track = Identifier.findTrack(uri, path);
    if (track != null) {
      return new TrackFile(uri, track);
    }
    return resolveDir(uri, path);
  }
  
  private VfsObject resolveDir(Uri uri, List<String> path) {
    final Author author = Identifier.findAuthor(uri, path);
    if (author != null) {
      return new AuthorDir(author);
    }
    final Genre genre = Identifier.findGenre(uri, path);
    if (genre != null) {
      return new GenreDir(genre);
    }
    final String category = Identifier.findCategory(path);
    if (category == null) {
      return this;
    }
    for (GroupingDir group : groupings) {
      if (category.equals(group.getPath())) {
        return group;
      }
    }
    return null;
  }

  private abstract class GroupingDir extends StubObject implements VfsDir {

    @Override
    public Uri getUri() {
      return Identifier.forCategory(getPath()).build();
    }

    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.SEARCH_ENGINE.equals(id) && catalog.searchSupported()) {
        //assume all the groups will search by authors
        return new AuthorsSearchEngine();
      } else {
        return super.getExtension(id);
      }
    }
    
    abstract String getPath();
  };

  private final class AuthorsDir extends GroupingDir {

    @Override
    public String getName() {
      return context.getString(R.string.vfs_modarchive_authors_name);
    }

    @Override
    public String getDescription() {
      return context.getString(R.string.vfs_modarchive_authors_description);
    }

    @Override
    public VfsObject getParent() {
      return VfsRootModarchive.this;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryAuthors(new Catalog.AuthorsVisitor() {
        @Override
        public void setCountHint(int count) {
          visitor.onItemsCount(count);
        }

        @Override
        public void accept(Author obj) {
          visitor.onDir(new AuthorDir(obj));
        }
      });
    }

    @Override
    public String getPath() {
      return Identifier.CATEGORY_AUTHOR;
    }
  }

  private final class GenresDir extends GroupingDir {

    @Override
    public String getName() {
      return context.getString(R.string.vfs_modarchive_genres_name);
    }

    @Override
    public String getDescription() {
      return context.getString(R.string.vfs_modarchive_genres_description);
    }

    @Override
    public VfsObject getParent() {
      return VfsRootModarchive.this;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryGenres(new Catalog.GenresVisitor() {

        @Override
        public void setCountHint(int hint) {
          visitor.onItemsCount(hint);
        }

        @Override
        public void accept(Genre genre) {
          visitor.onDir(new GenreDir(genre));
        }
      });
    }

    @Override
    public String getPath() {
      return Identifier.CATEGORY_GENRE;
    }
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
      return author.alias;
    }

    @Override
    public VfsObject getParent() {
      return groupings[0];
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryTracks(author, new Catalog.TracksVisitor() {
        @Override
        public void setCountHint(int count) {
          visitor.onItemsCount(count);
        }

        @Override
        public void accept(Track obj) {
          final Uri fileUri = Identifier.forTrack(Identifier.forAuthor(author), obj).build();
          visitor.onFile(new TrackFile(fileUri, obj));
        }
      });
    }
  }

  private class GenreDir extends StubObject implements VfsDir {

    private final Genre genre;

    GenreDir(Genre genre) {
      this.genre = genre;
    }

    @Override
    public Uri getUri() {
      return Identifier.forGenre(genre).build();
    }

    @Override
    public String getName() {
      return genre.name;
    }
    
    @Override
    public String getDescription() {
      return context.getResources().getQuantityString(R.plurals.tracks, genre.tracks, genre.tracks);
    }

    @Override
    public VfsObject getParent() {
      return groupings[1];//TODO
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryTracks(genre, new Catalog.TracksVisitor() {
        @Override
        public void setCountHint(int count) {
          visitor.onItemsCount(count);
        }

        @Override
        public void accept(Track obj) {
          final Uri fileUri = Identifier.forTrack(Identifier.forGenre(genre), obj).build();
          visitor.onFile(new TrackFile(fileUri, obj));
        }
      });
    }
  }
  
  private class TrackFile extends StubObject implements VfsFile {

    private final Uri uri;
    private final Track track;

    TrackFile(Uri uri, Track track) {
      this.uri = uri;
      this.track = track;
    }

    @Override
    public Uri getUri() {
      return uri;
    }

    @Override
    public String getName() {
      return track.filename;
    }
    
    @Override
    public String getDescription() {
      return track.title;
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
    public VfsObject getParent() {
      final List<String> path = uri.getPathSegments();
      return resolveDir(uri, path);
    }

    @Override
    public String getSize() {
      return Formatter.formatShortFileSize(context, track.size);
    }

    @Override
    public ByteBuffer getContent() throws IOException {
      return catalog.getTrackContent(track.id);
    }


    private String getShareUrl() {
      return String.format(Locale.US, "http://modarchive.org/index.php?request=view_player&query=%d",
          track.id);
    }
  }

  private class AuthorsSearchEngine implements VfsExtensions.SearchEngine {
    
    @Override
    public void find(String query, final Visitor visitor) throws IOException {
      catalog.findTracks(query, new Catalog.FoundTracksVisitor() {
        
        @Override
        public void accept(Author author, Track track) {
          final Uri.Builder authorsUri = Identifier.forAuthor(author);
          final Uri.Builder trackUri = Identifier.forTrack(authorsUri, track);
          visitor.onFile(new TrackFile(trackUri.build(), track));
        }
      });
    }
  }
}
