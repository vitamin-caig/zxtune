/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs.zxtunes;

import java.util.List;

import android.net.Uri;

/**
 * 
 * Paths
 * 
 * 1) zxtune:/
 * 2) zxtune:/${authors_folder}
 * 3) zxtune:/${authors_folder}/${author_name}?author=${author_id}
 * 4) zxtune:/${authors_folder}/${author_name}/${date}?author=${author_id}
 * 5) zxtune:/${authors_folder}/${author_name}/${Filename}?author=${author_id}&track=${track_id}
 * 6) zxtune:/${authors_folder}/${author_name}/${date}/${Filename}?author=${author_id}&track=${track_id}
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

public class Identifier {

  private final static String SCHEME = "zxtunes";

  private final static int POS_CATEGORY = 0;
  private final static int POS_AUTHOR_NICK = 1;
  private final static int POS_AUTHOR_DATE = 2;
  private final static int POS_AUTHOR_TRACK = 2;
  private final static int POS_AUTHOR_DATE_TRACK = 3;

  private final static String PARAM_AUTHOR_ID = "author";
  private final static String PARAM_TRACK_ID = "track";

  public final static String CATEGORY_AUTHORS = "Authors";
  
  // Root
  public static Uri.Builder forRoot() {
    return new Uri.Builder().scheme(SCHEME);
  }
  
  public static boolean isFromRoot(Uri uri) {
    return uri.getScheme().equals(SCHEME);
  }
  
  // Categories
  public static Uri.Builder forCategory(String category) {
    return forRoot().appendPath(category);
  }

  public static String findCategory(List<String> path) {
    return path.size() > POS_CATEGORY ? path.get(POS_CATEGORY) : null;
  }

  // Authors
  public static Uri.Builder forAuthor(Author author) {
    return forCategory(CATEGORY_AUTHORS)
        .appendPath(author.nickname)
        .appendQueryParameter(PARAM_AUTHOR_ID, Integer.toString(author.id));
  }

  public static Uri.Builder forAuthor(Author author, int date) {
    return forCategory(CATEGORY_AUTHORS)
        .appendPath(author.nickname)
        .appendPath(Integer.toString(date))
        .appendQueryParameter(PARAM_AUTHOR_ID, Integer.toString(author.id));
  }
  
  public static Author findAuthor(Uri uri, List<String> path) {
    if (path.size() > POS_AUTHOR_NICK) {
      final String nick = path.get(POS_AUTHOR_NICK);
      final String id = uri.getQueryParameter(PARAM_AUTHOR_ID);
      return new Author(Integer.valueOf(id), nick, ""/*fake*/);
    } else {
      return null;
    }
  }
  
  public static Integer findDate(Uri uri, List<String> path) {
    if (path.size() > POS_AUTHOR_DATE) {
      return Integer.valueOf(path.get(POS_AUTHOR_DATE));
    } else {
      return null;
    }
  }
  
  // Tracks
  public static Uri.Builder forTrack(Uri.Builder parent, Track track) {
    if (track.date != null) {
      parent.appendPath(track.date.toString());
    }
    return parent.appendPath(track.filename)
        .appendQueryParameter(PARAM_TRACK_ID, Integer.toString(track.id));
  }
  
  public static Track findTrack(Uri uri, List<String> path) {
    final String id = uri.getQueryParameter(PARAM_TRACK_ID);
    if (id == null) {
      return null;
    }
    if (path.size() > POS_AUTHOR_DATE_TRACK) {
      final String date = path.get(POS_AUTHOR_DATE);
      final String filename = path.get(POS_AUTHOR_DATE_TRACK);
      return new Track(Integer.valueOf(id), filename, null/*fake*/, null/*fake*/, Integer.valueOf(date));
    } else {
      final String filename = path.get(POS_AUTHOR_TRACK);
      return new Track(Integer.valueOf(id), filename, null/*fake*/, null/*fake*/, null);
    }
  }
}
