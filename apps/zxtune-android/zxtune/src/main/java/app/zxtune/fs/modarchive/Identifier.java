/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.modarchive;

import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.List;

/**
 *
 * Paths:
 *
 * 1) modarchive:/
 * 2) modarchive:/Author
 * 3) modarchive:/Author/${author_name}?author=${author_id}
 * 4) modarchive:/Genre
 * 5) modarchive:/Genre/${genre_name}?genre=${genre_id}
 * 6) modarchive:/Random
 */

public class Identifier {

  private static final String SCHEME = "modarchive";

  private static final int POS_CATEGORY = 0;
  private static final int POS_AUTHOR_NAME = 1;
  private static final int POS_GENRE_NAME = 1;
  private static final int POS_TRACK_FILENAME = 2;

  private static final String PARAM_AUTHOR = "author";
  private static final String PARAM_GENRE = "genre";
  private static final String PARAM_TRACK = "track";

  public static final String CATEGORY_AUTHOR = "Author";
  public static final String CATEGORY_GENRE = "Genre";
  public static final String CATEGORY_RANDOM = "Random";

  // Root
  public static Uri.Builder forRoot() {
    return new Uri.Builder().scheme(SCHEME);
  }

  public static boolean isFromRoot(Uri uri) {
    return SCHEME.equals(uri.getScheme());
  }

  // Categories
  public static Uri.Builder forCategory(String category) {
    return forRoot().appendPath(category);
  }

  @Nullable
  public static String findCategory(List<String> path) {
    return path.size() > POS_CATEGORY ? path.get(POS_CATEGORY) : null;
  }

  // Authors
  public static Uri.Builder forAuthor(Author author) {
    return forCategory(CATEGORY_AUTHOR)
            .appendPath(author.getAlias())
            .appendQueryParameter(PARAM_AUTHOR, String.valueOf(author.getId()));
  }

  @Nullable
  public static Author findAuthor(Uri uri, List<String> path) {
    if (path.size() > POS_AUTHOR_NAME) {
      final String id = uri.getQueryParameter(PARAM_AUTHOR);
      if (id != null) {
        final String name = path.get(POS_AUTHOR_NAME);
        return new Author(Integer.valueOf(id), name);
      }
    }
    return null;
  }

  // Genres
  public static Uri.Builder forGenre(Genre genre) {
    return forCategory(CATEGORY_GENRE)
            .appendPath(genre.name)
            .appendQueryParameter(PARAM_GENRE, String.valueOf(genre.id));
  }

  @Nullable
  public static Genre findGenre(Uri uri, List<String> path) {
    if (path.size() > POS_GENRE_NAME) {
      final String id = uri.getQueryParameter(PARAM_GENRE);
      if (id != null) {
        final String name = path.get(POS_GENRE_NAME);
        return new Genre(Integer.valueOf(id), name, 0/*fake*/);
      }
    }
    return null;
  }

  // Tracks
  public static Uri.Builder forTrack(Uri.Builder parent, Track track) {
    return parent.appendPath(track.filename).appendQueryParameter(PARAM_TRACK, String.valueOf(track.id));
  }

  @Nullable
  public static Track findTrack(Uri uri, List<String> path) {
    final int elements = path.size();
    if (elements >= 2) {
      // do not allow files on root
      final String id = uri.getQueryParameter(PARAM_TRACK);
      if (id != null) {
        final String name = path.get(elements - 1);
        return new Track(Integer.valueOf(id), name, ""/*fake*/, 0/*fake*/);
      }
    }
    return null;
  }
}
