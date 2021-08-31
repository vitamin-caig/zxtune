/**
 * @file
 * @brief Implementation of VfsRoot over http://amp.dascene.net catalogue
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.ArrayDeque;
import java.util.List;
import java.util.Locale;

import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.Util;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.fs.modarchive.Author;
import app.zxtune.fs.modarchive.CachingCatalog;
import app.zxtune.fs.modarchive.Catalog;
import app.zxtune.fs.modarchive.Genre;
import app.zxtune.fs.modarchive.Identifier;
import app.zxtune.fs.modarchive.RemoteCatalog;
import app.zxtune.fs.modarchive.Track;

@Icon(R.drawable.ic_browser_vfs_modarchive)
final class VfsRootModarchive extends StubObject implements VfsRoot {

  private static final String TAG = VfsRootModarchive.class.getName();

  private final VfsObject parent;
  private final Context context;
  private final CachingCatalog catalog;
  private final GroupingDir[] groupings;

  VfsRootModarchive(VfsObject parent, Context context, MultisourceHttpProvider http, String key) {
    this.parent = parent;
    this.context = context;
    this.catalog = Catalog.create(context, http, key);
    this.groupings = new GroupingDir[]{
            new AuthorsDir(),
            new GenresDir(),
            new RandomDir(),
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
    return parent;
  }

  @Override
  public Object getExtension(String id) {
    if (VfsExtensions.SEARCH_ENGINE.equals(id)) {
      //assume root will search by authors
      return new AuthorsSearchEngine();
    } else {
      return super.getExtension(id);
    }
  }

  @Override
  public void enumerate(Visitor visitor) {
    for (GroupingDir group : groupings) {
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
    // due to identical structure of groupings, may resolve here
    // use plain algo with most frequent cases check first
    final Track track = Identifier.findTrack(uri, path);
    if (track != null) {
      return new TrackFile(uri, track);
    } else {
      return resolveDir(uri, path);
    }
  }

  @Nullable
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
      if (VfsExtensions.SEARCH_ENGINE.equals(id)) {
        //assume all the groups will search by authors
        return new AuthorsSearchEngine();
      } else {
        return super.getExtension(id);
      }
    }

    abstract String getPath();
  }

  private final class AuthorsDir extends GroupingDir {

    @Override
    public String getName() {
      return context.getString(R.string.vfs_modarchive_authors_name);
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
      }, visitor);
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

  @Icon(R.drawable.ic_browser_vfs_radio)
  private final class RandomDir extends GroupingDir {

    @Override
    public String getName() {
      return context.getString(R.string.vfs_modarchive_random_name);
    }

    @Override
    public String getDescription() {
      return context.getString(R.string.vfs_modarchive_random_description);
    }

    @Override
    public VfsObject getParent() {
      return VfsRootModarchive.this;
    }

    @Override
    public String getPath() {
      return Identifier.CATEGORY_RANDOM;
    }

    @Override
    public void enumerate(Visitor visitor) {}

    @Override
    @Nullable
    public Object getExtension(String id) {
      if (VfsExtensions.FEED.equals(id)) {
        return new FeedIterator();
      } else {
        return super.getExtension(id);
      }
    }
  }

  private class FeedIterator implements java.util.Iterator<VfsFile> {

    // To avoid enless loop in case of fail, limit it by full cycles count
    private static final int MAX_REPEATS = 5;

    private final ArrayDeque<Track> tracks = new ArrayDeque<>();
    private int hash = 0;
    private int count = 0;

    @Override
    public boolean hasNext() {
      if (tracks.isEmpty()) {
        loadRandomTracks();
      }
      return !tracks.isEmpty();
    }

    @Override
    public VfsFile next() {
      final Track obj = tracks.removeFirst();
      final Uri fileUri = Identifier.forTrack(Identifier.forCategory(Identifier.CATEGORY_RANDOM), obj).build();
      return new TrackFile(fileUri, obj);
    }

    private void loadRandomTracks() {
      final int[] newHash = new int[1];
      try {
        catalog.findRandomTracks(new Catalog.TracksVisitor() {
          // TODO: remove
          @Override
          public void setCountHint(int count) {}

          @Override
          public void accept(Track obj) {
            tracks.addLast(obj);
            // use associative hashing to cover random ordered result
            newHash[0] += obj.getId();
          }
        });
      } catch (IOException e) {
        Log.w(TAG, e,"Failed to load random tracks");
      }
      if (hash == newHash[0]) {
        if (++count > MAX_REPEATS) {
          Log.d(TAG, "Break feed loop");
          tracks.clear();
        }
      } else {
        hash = newHash[0];
        count = 1;
      }
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
      return author.getAlias();
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
      }, visitor);
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
      return genre.getName();
    }

    @Override
    public String getDescription() {
      return context.getResources().getQuantityString(R.plurals.tracks, genre.getTracks(), genre.getTracks());
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
      }, visitor);
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
      return track.getFilename();
    }

    @Override
    public String getDescription() {
      return track.getTitle();
    }

    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.CACHE_PATH.equals(id)) {
        return Integer.toString(track.getId());
      } else if (VfsExtensions.DOWNLOAD_URIS.equals(id)) {
        return RemoteCatalog.getTrackUris(track.getId());
      } else if (VfsExtensions.SHARE_URL.equals(id)) {
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
      return Util.formatSize(track.getSize());
    }

    private String getShareUrl() {
      return String.format(Locale.US, "https://modarchive.org/index" +
              ".php?request=view_player&query=%d",
              track.getId());
    }
  }

  private class AuthorsSearchEngine implements VfsExtensions.SearchEngine {

    @Override
    public void find(String query, final Visitor visitor) throws IOException {
      catalog.findTracks(query, new Catalog.FoundTracksVisitor() {

        // TODO: remove
        @Override
        public void setCountHint(int count) {}

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
