/**
 * @file
 * @brief Implementation of VfsRoot over http://zxtunes.com catalogue
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import android.content.Context;
import android.net.Uri;
import android.util.SparseIntArray;

import androidx.annotation.Nullable;

import java.io.IOException;
import java.util.List;
import java.util.Locale;

import app.zxtune.R;
import app.zxtune.TimeStamp;
import app.zxtune.fs.http.MultisourceHttpProvider;
import app.zxtune.fs.zxtunes.Author;
import app.zxtune.fs.zxtunes.CachingCatalog;
import app.zxtune.fs.zxtunes.Catalog;
import app.zxtune.fs.zxtunes.Identifier;
import app.zxtune.fs.zxtunes.RemoteCatalog;
import app.zxtune.fs.zxtunes.Track;

final class VfsRootZxtunes extends StubObject implements VfsRoot {

  private static final String TAG = VfsRootZxtunes.class.getName();

  private final VfsObject parent;
  private final Context context;
  private final CachingCatalog catalog;
  private final GroupingDir[] groups;

  VfsRootZxtunes(VfsObject parent, Context context, MultisourceHttpProvider http) {
    this.parent = parent;
    this.context = context;
    this.catalog = Catalog.create(context, http);
    this.groups = new GroupingDir[]{new AuthorsDir()};
  }

  @Override
  public Uri getUri() {
    return Identifier.forRoot().build();
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
  public VfsObject getParent() {
    return parent;
  }

  @Override
  public Object getExtension(String id) {
    if (VfsExtensions.SEARCH_ENGINE.equals(id) && catalog.searchSupported()) {
      //assume root will search by authors
      return new AuthorsSearchEngine();
    } else if (VfsExtensions.ICON.equals(id)) {
      return R.drawable.ic_browser_vfs_zxtunes;
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
    } else if (Identifier.CATEGORY_IMAGES.equals(category)) {
      return resolveImages(uri, path);
    } else {
      for (GroupingDir group : groups) {
        if (category.equals(group.getPath())) {
          return group.resolve(uri, path);
        }
      }
    }
    return null;
  }

  @Nullable
  private VfsFile resolveImages(Uri uri, List<String> path) {
    final Author author = Identifier.findAuthor(uri, path);
    if (author != null) {
      return new ImageFile(author);
    }
    return null;
  }

  private abstract class GroupingDir extends StubObject implements VfsDir {

    @Override
    public Uri getUri() {
      return Identifier.forCategory(getPath()).build();
    }

    abstract String getPath();

    abstract VfsObject resolve(Uri uri, List<String> path);
  }

  private class AuthorsDir extends GroupingDir {

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
    public Object getExtension(String id) {
      if (VfsExtensions.SEARCH_ENGINE.equals(id) && catalog.searchSupported()) {
        return new AuthorsSearchEngine();
      } else {
        return super.getExtension(id);
      }
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryAuthors((obj) -> {
        visitor.onDir(new AuthorDir(obj));
      });
    }

    @Override
    public String getPath() {
      return Identifier.CATEGORY_AUTHORS;
    }

    @Override
    final VfsObject resolve(Uri uri, List<String> path) {
      // use plain resolving with most frequent cases first
      final Track track = Identifier.findTrack(uri, path);
      if (track != null) {
        return new TrackFile(uri, track);
      }
      final VfsObject dir = resolveDir(uri, path);
      return dir != null ? dir : this;
    }
  }

  @Nullable
  private VfsObject resolveDir(Uri uri, List<String> path) {
    final Author author = Identifier.findAuthor(uri, path);
    if (author == null) {
      return null;
    }
    final Integer date = Identifier.findDate(uri, path);
    return date != null ? new AuthorDateDir(author, date) : new AuthorDir(author);
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
      // TODO
      return groups[0];
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      final SparseIntArray dates = new SparseIntArray();
      catalog.queryAuthorTracks(author, (obj) -> {
        if (isEmptyDate(obj.getDate())) {
          final Uri uri = Identifier.forTrack(Identifier.forAuthor(author), obj).build();
          visitor.onFile(new TrackFile(uri, obj));
        } else {
          dates.put(obj.getDate(), 1 + dates.get(obj.getDate()));
        }
      });
      for (int i = 0, lim = dates.size(); i != lim; ++i) {
        visitor.onDir(new AuthorDateDir(author, dates.keyAt(i), dates.valueAt(i)));
      }
    }

    @Override
    public Object getExtension(String id) {
      if (VfsExtensions.COVER_ART_URI.equals(id)) {
        final Author obj = catalog.queryAuthor(author.getId());
        return obj != null && Boolean.TRUE == obj.getHasPhoto() ? Identifier.forPhotoOf(author) : null;
      } else {
        return super.getExtension(id);
      }
    }
  }

  private static boolean isEmptyDate(@Nullable Integer date) {
    return date == null || 0 == date;
  }

  private class AuthorDateDir extends StubObject implements VfsDir {

    private final Author author;
    private final int date;
    private final int count;

    AuthorDateDir(Author author, int date, int count) {
      this.author = author;
      this.date = date;
      this.count = count;
    }

    AuthorDateDir(Author author, int date) {
      this(author, date, 0);
    }

    @Override
    public Uri getUri() {
      return Identifier.forAuthor(author, date).build();
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
      catalog.queryAuthorTracks(author, (obj) -> {
        if (!isEmptyDate(obj.getDate()) && date == obj.getDate()) {
          final Uri uri = Identifier.forTrack(Identifier.forAuthor(author), obj).build();
          visitor.onFile(new TrackFile(uri, obj));
        }
      });
    }
  }

  private static final TimeStamp FRAME_DURATION = TimeStamp.fromMilliseconds(20);

  private class TrackFile extends StubObject implements VfsFile {

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
      return module.getTitle();
    }

    @Override
    public VfsObject getParent() {
      final List<String> path = uri.getPathSegments();
      return resolveDir(uri, path);
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
      return module.getDuration() != null ? FRAME_DURATION.multiplies(module.getDuration()).toString() : "";
    }

    @Nullable
    private String getShareUrl() {
      final Author author = Identifier.findAuthor(uri, uri.getPathSegments());
      return author != null ? String.format(Locale.US, "https://zxtunes.com/author.php?id=%d&play=%d", author.getId(), module.getId()) : null;
    }
  }

  private static class ImageFile extends StubObject implements VfsFile {
    private final Author author;

    private ImageFile(Author author) {
      this.author = author;
    }

    @Override
    public Uri getUri() {
      return Identifier.forPhotoOf(author);
    }

    @Override
    public String getName() {
      return author.getNickname();
    }

    @Override
    public VfsObject getParent() {
      return null;
    }

    @Override
    public String getSize() {
      return "";
    }

    @Override
    @Nullable
    public Object getExtension(String id) {
      if (VfsExtensions.CACHE_PATH.equals(id)) {
        return author.getId() + ".jpg";
      } else if (VfsExtensions.DOWNLOAD_URIS.equals(id)) {
        return RemoteCatalog.getImageUris(author.getId());
      } else {
        return super.getExtension(id);
      }
    }
  }

  private class AuthorsSearchEngine implements VfsExtensions.SearchEngine {

    @Override
    public void find(String query, final Visitor visitor) {
      catalog.findTracks(query, (author, track) -> {
        final Uri uri = Identifier.forTrack(Identifier.forAuthor(author), track).build();
        visitor.onFile(new TrackFile(uri, track));
      });
    }
  }
}
