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

/**
 *
 * Paths:
 *
 * 1) amp:/
 * 2) amp:/Handle
 * 3) amp:/Handle/${author_letter}
 * 4) amp:/Handle/${author_letter}/${author_name}?id=${id}
 * 5) amp:/Country
 * 6) amp:/Country/${country}?country=${country}
 * 7) amp:/Country/${country}/${author_name}?id=${id}&country=${country}
 */

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;
import java.util.Locale;
import java.util.concurrent.atomic.AtomicReference;

import android.content.Context;
import android.net.Uri;
import android.text.format.Formatter;
import app.zxtune.Log;
import app.zxtune.R;
import app.zxtune.fs.amp.Author;
import app.zxtune.fs.amp.Catalog;
import app.zxtune.fs.amp.Country;
import app.zxtune.fs.amp.Track;
import app.zxtune.ui.IconSource;

final class VfsRootAmp implements VfsRoot, IconSource {

  private final static String TAG = VfsRootAmp.class.getName();

  private final static String SCHEME = "amp";

  private final static int POS_CATEGORY = 0;
  private final static int POS_LETTER = 1;
  private final static int POS_COUNTRY = 1;
  private final static int POS_NAME = 2;
  //private final static int POS_FILENAME = 3;

  private final static String PARAM_AUTHOR = "author";
  private final static String PARAM_COUNTRY = "country";
  private final static String PARAM_TRACK = "track";

  private final Context context;
  private final Catalog catalog;
  private final GroupsDir groups[];

  VfsRootAmp(Context context) {
    this.context = context;
    this.catalog = Catalog.create(context);
    this.groups = new GroupsDir[] {
      new HandlesDir(),
      new CountriesDir()
    };
  }

  @Override
  public Uri getUri() {
    return rootUri().build();
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
  public void enumerate(Visitor visitor) throws IOException {
    for (GroupsDir group : groups) {
      visitor.onDir(group);
    }
  }

  @Override
  public VfsObject resolve(Uri uri) throws IOException {
    if (SCHEME.equals(uri.getScheme())) {
      return resolvePath(uri);
    }
    return null;
  }

  @Override
  public int getResourceId() {
    return R.drawable.ic_browser_vfs_amp;
  }
  
  private Uri.Builder rootUri() {
    return new Uri.Builder().scheme(SCHEME);
  }

  private VfsObject resolvePath(Uri uri) throws IOException {
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
  
  private abstract class GroupsDir extends StubObject implements VfsDir {
    abstract String getPath();
    abstract VfsObject resolve(Uri uri, List<String> path) throws IOException;
  };
  
  private static boolean isLetter(char c) {
    return (Character.isLetter(c) && Character.isUpperCase(c));
  }
  
  private Author queryAuthor(int id) throws IOException {
    final AtomicReference<Author> result = new AtomicReference<Author>();
    catalog.queryAuthors(id, new Catalog.AuthorsVisitor() {
      @Override
      public void accept(Author obj) {
        result.set(obj);
      }
    });
    return result.get();
  }
  
  private final class HandlesDir extends GroupsDir {
  
    @Override
    public Uri getUri() {
      return handlesUri().build();
    }
    
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
      return "Handle";
    }
    
    @Override
    final VfsObject resolve(Uri uri, List<String> path) throws IOException {
      if (POS_CATEGORY == path.size() - 1) {
        return this;
      } else {
        final String letter = Uri.decode(path.get(POS_LETTER));
        if (letter.equals(Catalog.NON_LETTER_FILTER) || (letter.length() == 1 && isLetter(letter.charAt(0)))) {
          return new HandleByLetterDir(letter).resolve(uri, path);
        } else {
          return null;
        }
      }
    }
    
    private final Uri.Builder handlesUri() {
      return rootUri().appendPath(getPath());
    }

    private class HandleByLetterDir extends StubObject implements VfsDir {

      private final String letter;

      HandleByLetterDir(String letter) {
        this.letter = letter;
      }

      @Override
      public Uri getUri() {
        return handlesLetterUri().build();
      }

      @Override
      public String getName() {
        return letter;
      }

      @Override
      public VfsObject getParent() {
        return HandlesDir.this;
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
            visitor.onDir(new AuthorDir(HandleByLetterDir.this, obj));
          }
        });
      }

      final VfsObject resolve(Uri uri, List<String> path) throws IOException {
        if (POS_LETTER == path.size() - 1) {
          return this;
        } else {
          try {
            final int id = Integer.parseInt(uri.getQueryParameter(PARAM_AUTHOR));
            final Author author = queryAuthor(id);
            return author != null
                    ? new AuthorDir(this, author).resolve(uri, path)
                    : null;
          } catch (IOException e) {
            throw e;
          } catch (Exception e) {
            Log.d(TAG, e, "resolve %s", uri);
            return null;
          }
        }
      }

      final Uri.Builder handlesLetterUri() {
        return handlesUri().appendPath(letter);
      }
    }
  }
  
  private final class CountriesDir extends GroupsDir {
    
    private final String[] countries;
    
    CountriesDir() {
      this.countries = context.getResources().getStringArray(R.array.vfs_amp_countries_list); 
    }
    
    @Override
    public Uri getUri() {
      return countriesUri().build();
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
      return "Country";
    }
    
    @Override
    final VfsObject resolve(Uri uri, List<String> path) throws IOException {
      if (POS_CATEGORY == path.size() - 1) {
        return this;
      } else {
        final String countryId = uri.getQueryParameter(PARAM_COUNTRY);
        if (countryId != null) {
          try {
            final int UNKNOWN = 60;
            final int id = Integer.parseInt(countryId);
            final String name = id > 0 && id < countries.length ? countries[id] : countries[UNKNOWN];
            final Country country = new Country(id, name);
            return new HandleByCountryDir(country).resolve(uri, path);
          } catch (NumberFormatException e) {
            Log.d(TAG, "Unknown countryid=%d", countryId);
          }
        }
        return null;
      }
    }
    
    private final Uri.Builder countriesUri() {
      return rootUri().appendPath(getPath());
    }

    private class HandleByCountryDir extends StubObject implements VfsDir {

      private final Country country;

      HandleByCountryDir(Country country) {
        this.country = country;
      }

      @Override
      public Uri getUri() {
        return handlesCountryUri().build();
      }

      @Override
      public String getName() {
        return country.name;
      }

      @Override
      public VfsObject getParent() {
        return CountriesDir.this;
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
            visitor.onDir(new AuthorDir(HandleByCountryDir.this, obj));
          }
        });
      }

      final VfsObject resolve(Uri uri, List<String> path) throws IOException {
        if (POS_COUNTRY == path.size() - 1) {
          return this;
        } else {
          try {
            final int id = Integer.parseInt(uri.getQueryParameter(PARAM_AUTHOR));
            final Author author = queryAuthor(id);
            return author != null
                ? new AuthorDir(this, author).resolve(uri, path)
                : null;
          } catch (IOException e) {
            throw e;
          } catch (Exception e) {
            Log.d(TAG, e, "resolve %s", uri);
            return null;
          }
        }
      }

      final Uri.Builder handlesCountryUri() {
        return countriesUri()
            .appendPath(country.name)
            .appendQueryParameter(PARAM_COUNTRY, String.valueOf(country.id));
      }
    }
  }

  private class AuthorDir implements VfsDir {

    private final VfsObject parent;
    private final Author author;
    private final Uri.Builder uri;

    AuthorDir(VfsObject parent, Author author) {
      this.parent = parent;
      this.author = author;
      this.uri = buildUri();
    }

    @Override
    public Uri getUri() {
      return uri.build();
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
      return parent;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryTracks(author, null, new Catalog.TracksVisitor() {
        @Override
        public void setCountHint(int count) {
          visitor.onItemsCount(count);
        }
        
        @Override
        public void accept(Track obj) {
          visitor.onFile(new TrackFile(obj));
        }
      });
    }

    final VfsObject resolve(Uri uri, List<String> path) throws IOException {
      if (POS_NAME == path.size() - 1) {
        return this;
      } else {
        try {
          final Integer id = Integer.parseInt(uri.getQueryParameter(PARAM_TRACK));
          final AtomicReference<VfsObject> result = new AtomicReference<VfsObject>();
          catalog.queryTracks(author, id, new Catalog.TracksVisitor() {
            @Override
            public void accept(Track obj) {
              result.set(new TrackFile(obj));
            }
          });
          return result.get();
        } catch (IOException e) {
          throw e;
        } catch (Exception e) {
          Log.d(TAG, e, "resolve %s", uri);
          return null;
        }
      }
    }
    
    private Uri.Builder buildUri() {
      return parent.getUri().buildUpon()
          .appendPath(getName())
          .appendQueryParameter(PARAM_AUTHOR, String.valueOf(author.id));
    }

    private class TrackFile extends StubObject implements VfsFile {
      
      private final Track track;

      TrackFile(Track track) {
        this.track = track;
      }
      
      @Override
      public Uri getUri() {
        return buildUri()
            .appendPath(getName())
            .appendQueryParameter(PARAM_TRACK, String.valueOf(track.id))
            .build();
      }
      
      @Override
      public String getName() {
        return track.filename;
      }
      
      @Override
      public VfsObject getParent() {
        return AuthorDir.this;
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
}
