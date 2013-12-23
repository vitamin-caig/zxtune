/**
 *
 * @file
 *
 * @brief Implementation of VfsRoot over ftp://ftp.modland.com catalogue
 *
 * @author vitamin.caig@gmail.com
 *
 */

package app.zxtune.fs;

/**
 *
 * Paths:
 *
 * 1) modland:/
 * 2) modland:/Authors
 * 3) modland:/Authors/${author_letter}
 * 4) modland:/Authors/${author_letter}/${author_name}?id=${id}
 * 5) modland:/Collections
 * 6) modland:/Collections/${collection_letter}
 * 7) modland:/Collections/${collection_letter}/${collection_name}?id=${id}
 */

import android.content.Context;
import android.net.Uri;
import android.text.format.Formatter;
import android.util.Log;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.List;

import app.zxtune.R;
import app.zxtune.fs.modland.Author;
import app.zxtune.fs.modland.Catalog;
import app.zxtune.fs.modland.Track;

final class VfsRootModland implements VfsRoot {

  private final static String TAG = VfsRootModland.class.getName();

  private final static String SCHEME = "modland";

  private static class Storage {
    private final static String SCHEME = "ftp";
    private final static String HOST = "ftp.modland.com";

    static boolean checkUri(Uri uri) {
      return SCHEME.equals(uri.getScheme()) && HOST.equals(uri.getHost());
    }

    static Uri.Builder makeUri() {
      return new Uri.Builder().scheme(SCHEME)
        .authority(HOST);
    }
  }

  private final static int POS_CATEGORY = 0;
  private final static int POS_LETTER = 1;
  private final static int POS_NAME = 2;

  private final static String PARAM_ID = "id";

  private final static String NOT_LETTER = "#";

  private final Context context;
  private final Catalog catalog;
  private final AuthorsDir authors;

  VfsRootModland(Context context) {
    this.context = context;
    this.catalog = Catalog.create(context);
    this.authors = new AuthorsDir();
  }

  @Override
  public Uri getUri() {
    return rootUri().build();
  }

  @Override
  public String getName() {
    return context.getString(R.string.vfs_modland_root_name);
  }

  @Override
  public String getDescription() {
    return context.getString(R.string.vfs_modland_root_description);
  }

  @Override
  public VfsDir getParent() {
    return null;
  }

  @Override
  public void enumerate(Visitor visitor) throws IOException {
    visitor.onDir(authors);
  }

  @Override
  public void find(String mask, Visitor visitor) {
    //TODO
  }

  @Override
  public VfsObject resolve(Uri uri) throws IOException {
    if (SCHEME.equals(uri.getScheme())) {
      return resolvePath(uri);
    } else if (Storage.checkUri(uri)) {
      final Track track = new Track(uri.getEncodedPath(), 0);
      return new TrackFile(track);
    }
    return null;
  }

  private Uri.Builder rootUri() {
    return new Uri.Builder().scheme(SCHEME);
  }

  private Uri.Builder authorsUri() {
    return rootUri().appendPath(AuthorsDir.PATH);
  }

  private Uri.Builder authorLetterUri(String letter) {
    return authorsUri().appendPath(letter);
  }

  private Uri.Builder authorUri(String name, int id) {
    return authorLetterUri(getLetter(name))
      .appendPath(name)
      .appendQueryParameter(PARAM_ID, String.valueOf(id));
  }

  private Uri.Builder trackUri(String path) {
    return Storage.makeUri().encodedPath(path);
  }

  private boolean isLetter(char c) {
    return (Character.isLetter(c) && Character.isUpperCase(c));
  }

  private String getLetter(String str) {
    final char c = str.charAt(0);
    return isLetter(c) ? str.substring(0, 1) : NOT_LETTER;
  }

  private VfsObject resolvePath(Uri uri) {
    final List<String> path = uri.getPathSegments();
    if (path.isEmpty()) {
      return this;
    } else {
      final String category = path.get(POS_CATEGORY);
      if (category.equals(AuthorsDir.PATH)) {
        return authors.resolve(uri, path);
      } else {
        return null;
      }
    }
  }

  private class AuthorsDir implements VfsDir {

    // Static locale-independent path dir
    final static String PATH = "Authors";

    @Override
    public Uri getUri() {
      return authorsUri().build();
    }

    @Override
    public String getName() {
      return context.getString(R.string.vfs_modland_authors_name);
    }

    @Override
    public String getDescription() {
      return context.getString(R.string.vfs_modland_authors_description);
    }

    @Override
    public VfsDir getParent() {
      return VfsRootModland.this;
    }

    @Override
    public void enumerate(Visitor visitor) throws IOException {
      visitor.onDir(new AuthorByLetterDir(NOT_LETTER));
      for (char c = 'A'; c <= 'Z'; ++c) {
        visitor.onDir(new AuthorByLetterDir(String.valueOf(c)));
      }
    }

    @Override
    public void find(String mask, Visitor visitor) {
      //TODO
    }

    VfsObject resolve(Uri uri, List<String> path) {
      if (POS_CATEGORY == path.size() - 1) {
        return this;
      } else {
        final String letter = Uri.decode(path.get(POS_LETTER));
        if (letter.equals(NOT_LETTER) || (letter.length() == 1 && isLetter(letter.charAt(0)))) {
          return new AuthorByLetterDir(letter).resolve(uri, path);
        } else {
          return null;
        }
      }
    }
  }

  private class AuthorByLetterDir implements VfsDir {

    private String letter;

    AuthorByLetterDir(String letter) {
      this.letter = letter;
    }

    @Override
    public Uri getUri() {
      return authorLetterUri(letter).build();
    }

    @Override
    public String getName() {
      return letter;
    }

    @Override
    public String getDescription() {
      return "".intern();
    }

    @Override
    public VfsDir getParent() {
      return authors;
    }

    @Override
    public void enumerate(final Visitor visitor) throws IOException {
      catalog.queryAuthors(letter, new Catalog.AuthorsVisitor() {
        @Override
        public void accept(Author obj) {
          visitor.onDir(new AuthorDir(obj));
        }
      });
    }

    @Override
    public void find(String mask, Visitor visitor) {
      //TODO
    }

    VfsObject resolve(Uri uri, List<String> path) {
      if (POS_LETTER == path.size() - 1) {
        return this;
      } else {
        try {
          final int id = Integer.parseInt(uri.getQueryParameter(PARAM_ID));
          final Author author = catalog.queryAuthor(id);
          return author != null
            ? new AuthorDir(author).resolve(uri, path)
            : null;
        } catch (Exception e) {
          Log.d(TAG, "resolve(" + uri + ")", e);
          return null;
        }
      }
    }

    private class AuthorDir implements VfsDir {

      private Author author;

      AuthorDir(Author author) {
        this.author = author;
      }

      @Override
      public Uri getUri() {
        return authorUri(author.nickname, author.id).build();
      }

      @Override
      public String getName() {
        return author.nickname;
      }

      @Override
      public String getDescription() {
        return context.getResources().getQuantityString(R.plurals.tracks, author.tracks, author.tracks);
      }

      @Override
      public VfsDir getParent() {
        return AuthorByLetterDir.this;
      }

      @Override
      public void enumerate(final Visitor visitor) throws IOException {
        catalog.queryAuthorTracks(author.id, new Catalog.TracksVisitor() {
          @Override
          public void accept(Track obj) {
            visitor.onFile(new TrackFile(obj));
          }
        });
      }

      @Override
      public void find(String mask, Visitor visitor) {
        //TODO
      }

      VfsObject resolve(Uri uri, List<String> path) {
        if (POS_NAME == path.size() - 1) {
          return this;
        } else {
          return null;
        }
      }
    }
  }

  private class TrackFile implements VfsFile {

    private final Track track;

    TrackFile(Track track) {
      this.track = track;
    }

    @Override
    public Uri getUri() {
      return trackUri(track.path).build();
    }

    @Override
    public String getName() {
      return track.filename;
    }

    @Override
    public String getDescription() {
      return "".intern();
    }

    @Override
    public String getSize() {
      return Formatter.formatShortFileSize(context, track.size);
    }

    @Override
    public ByteBuffer getContent() throws IOException {
      return catalog.getTrackContent(track.path);
    }
  }
}
