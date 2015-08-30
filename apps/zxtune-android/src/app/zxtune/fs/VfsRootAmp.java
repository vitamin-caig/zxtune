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

import android.content.Context;
import android.net.Uri;
import android.text.format.Formatter;
import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.fs.amp.Author;
import app.zxtune.fs.amp.Catalog;
import app.zxtune.fs.amp.Catalog.GroupsVisitor;
import app.zxtune.fs.amp.Country;
import app.zxtune.fs.amp.Group;
import app.zxtune.fs.amp.Identifier;
import app.zxtune.fs.amp.Track;

final class VfsRootAmp extends StubObject implements VfsRoot {

  private final static String TAG = VfsRootAmp.class.getName();

  private final Context context;
  private final Catalog catalog;
  private final GroupingDir groupings[];

  VfsRootAmp(Context context) {
    this.context = context;
    this.catalog = Catalog.create(context);
    this.groupings = new GroupingDir[] {
        new HandlesDir(),
        new CountriesDir(),
        new GroupsDir()
    };
  }

  @Override
  public Uri getUri() {
    return Identifier.forRoot().build();
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_amp_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_amp_root_description);
  }

  @Override
  public VfsObject getParent() {
    return null;
  }
  
  @Override
  public Object getExtension(String id) {
    if (VfsExtensions.ICON_RESOURCE.equals(id)) {
      return R.drawable.ic_browser_vfs_amp;
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

  private VfsObject resolve(Uri uri, List<String> path) throws IOException {
    // due to identical structure of groupings, may resolve here
    // use plain algo with most frequent cases check first
    final Track track = Identifier.findTrack(uri, path);
    if (track != null) {
      return new TrackFile(uri, track);
    }
    final Author author = Identifier.findAuthor(uri, path);
    if (author != null) {
      return new AuthorDir(uri, author);
    }
    final VfsObject grouping = resolveGroupingDir(uri, path);
    if (grouping != null) {
      return grouping;
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

  private VfsObject resolveGroupingDir(Uri uri, List<String> path) {
    final Group group = Identifier.findGroup(uri, path);
    if (group != null) {
      return new HandleByGroupDir(group);
    }
    final Country country = Identifier.findCountry(uri, path);
    if (country != null) {
      return new HandleByCountryDir(country);
    }
    final String letter = Identifier.findHandleLetter(uri, path);
    if (letter != null) {
      return new HandleByLetterDir(letter);
    }
    return null;
  }

  private abstract class GroupingDir extends StubObject implements VfsDir {

    @Override
    public Uri getUri() {
      return Identifier.forCategory(getPath()).build();
    }

    abstract String getPath();
  };

  private abstract class HandleByGroupingDir extends StubObject implements VfsDir {

    final AuthorDir makeSubdir(Author author) {
      final Uri uri = Identifier.forAuthor(getUri().buildUpon(), author).build();
      return new AuthorDir(uri, author);
    }
  }

  private final class HandlesDir extends GroupingDir {

    @Override
    public String getName() {
      return context.getString(R.string.vfs_amp_handles_name);
    }

    @Override
    public String getDescription() {
      return context.getString(R.string.vfs_amp_handles_description);
    }

    @Override
    public VfsObject getParent() {
      return VfsRootAmp.this;
    }

    @Override
    public void enumerate(Visitor visitor) {
      visitor.onDir(new HandleByLetterDir(Catalog.NON_LETTER_FILTER));
      for (char c = 'A'; c <= 'Z'; ++c) {
        visitor.onDir(new HandleByLetterDir(String.valueOf(c)));
      }
    }

    @Override
    public String getPath() {
      return Identifier.CATEGORY_HANDLE;
    }
  }

  private class HandleByLetterDir extends HandleByGroupingDir {

    private final String letter;

    HandleByLetterDir(String letter) {
      this.letter = letter;
    }

    @Override
    public Uri getUri() {
      return Identifier.forHandleLetter(letter).build();
    }

    @Override
    public String getName() {
      return letter;
    }

    @Override
    public VfsObject getParent() {
      return groupings[0];// TODO
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryAuthors(letter, new Catalog.AuthorsVisitor() {
        @Override
        public void setCountHint(int count) {
          visitor.onItemsCount(count);
        }

        @Override
        public void accept(Author obj) {
          visitor.onDir(makeSubdir(obj));
        }
      });
    }
  }

  private final class CountriesDir extends GroupingDir {

    private final String[] countries;

    CountriesDir() {
      this.countries = context.getResources().getStringArray(R.array.vfs_amp_countries_list);
    }

    @Override
    public String getName() {
      return context.getString(R.string.vfs_amp_countries_name);
    }

    @Override
    public String getDescription() {
      return context.getString(R.string.vfs_amp_countries_description);
    }

    @Override
    public VfsObject getParent() {
      return VfsRootAmp.this;
    }

    @Override
    public void enumerate(Visitor visitor) {
      for (int id = 0; id < countries.length; ++id) {
        final String name = countries[id];
        if (!name.isEmpty()) {
          visitor.onDir(new HandleByCountryDir(new Country(id, name)));
        }
      }
    }

    @Override
    public String getPath() {
      return Identifier.CATEGORY_COUNTRY;
    }
  }

  private class HandleByCountryDir extends HandleByGroupingDir {

    private final Country country;

    HandleByCountryDir(Country country) {
      this.country = country;
    }

    @Override
    public Uri getUri() {
      return Identifier.forCountry(country).build();
    }

    @Override
    public String getName() {
      return country.name;
    }

    @Override
    public VfsObject getParent() {
      return groupings[1];// TODO
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryAuthors(country, new Catalog.AuthorsVisitor() {
        @Override
        public void setCountHint(int count) {
          visitor.onItemsCount(count);
        }

        @Override
        public void accept(Author obj) {
          visitor.onDir(makeSubdir(obj));
        }
      });
    }
  }

  private final class GroupsDir extends GroupingDir {

    @Override
    public String getName() {
      return context.getString(R.string.vfs_amp_groups_name);
    }

    @Override
    public String getDescription() {
      return context.getString(R.string.vfs_amp_groups_description);
    }

    @Override
    public VfsObject getParent() {
      return VfsRootAmp.this;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryGroups(new GroupsVisitor() {

        @Override
        public void setCountHint(int hint) {
          visitor.onItemsCount(hint);
        }

        @Override
        public void accept(Group group) {
          visitor.onDir(new HandleByGroupDir(group));
        }
      });
    }

    @Override
    public String getPath() {
      return Identifier.CATEGORY_GROUP;
    }
  }

  private class HandleByGroupDir extends HandleByGroupingDir {

    private final Group group;

    HandleByGroupDir(Group group) {
      this.group = group;
    }

    @Override
    public Uri getUri() {
      return Identifier.forGroup(group).build();
    }

    @Override
    public String getName() {
      return group.name;
    }

    @Override
    public VfsObject getParent() {
      return groupings[2];// TODO
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryAuthors(group, new Catalog.AuthorsVisitor() {
        @Override
        public void setCountHint(int count) {
          visitor.onItemsCount(count);
        }

        @Override
        public void accept(Author obj) {
          visitor.onDir(makeSubdir(obj));
        }
      });
    }
  }

  private class AuthorDir extends StubObject implements VfsDir {

    private final Uri uri;
    private final Author author;

    AuthorDir(Uri uri, Author author) {
      this.uri = uri;
      this.author = author;
    }

    @Override
    public Uri getUri() {
      return uri;
    }

    @Override
    public String getName() {
      return author.handle;
    }

    @Override
    public String getDescription() {
      return author.realName;
    }

    @Override
    public VfsObject getParent() {
      final List<String> path = uri.getPathSegments();
      return resolveGroupingDir(uri, path);
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
          final Uri fileUri = Identifier.forTrack(uri.buildUpon(), obj).build();
          visitor.onFile(new TrackFile(fileUri, obj));
        }
      });
    }
  }

  private VfsObject resolveAuthorDir(Uri uri, List<String> path) {
    final Author author = Identifier.findAuthor(uri, path);
    if (author == null) {
      return null;
    }
    final VfsObject grouping = resolveGroupingDir(uri, path);
    if (grouping == null) {
      return null;
    }
    // cut uri here
    return new AuthorDir(Identifier.forAuthor(grouping.getUri().buildUpon(), author).build(),
        author);
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
    public VfsObject getParent() {
      final List<String> path = uri.getPathSegments();
      return resolveAuthorDir(uri, path);
    }

    @Override
    public String getSize() {
      return Formatter.formatShortFileSize(context, track.size * 1024);
    }

    @Override
    public ByteBuffer getContent() throws IOException {
      return catalog.getTrackContent(track.id);
    }
  }
}
