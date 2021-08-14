/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.zxart;

import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.List;

/**
 * Paths:
 *
 * zxart:
 *
 * zxart://Authors
 * zxart://Authors/${nick}?author=${author_id}
 * zxart://Authors/${nick}/${year}?author=${author_id}
 * zxart://Authors/${nick}/${year}/${filename}?author=${author_id}&track=${track_id}
 *
 * zxart://Parties
 * zxart://Parties/${year}
 * zxart://Parties/${year}/${name}?party=${party_id}
 * zxart://Parties/${year}/${name}/${compo}?party=${party_id}
 * zxart://Parties/${year}/${name}/${compo}/${filename}?party=${party_id}&track=${track_id}
 *
 * zxart://Top
 * zxart://Top/${filename}?track=${track_id}
 *
 */

public class Identifier {

  private static final String SCHEME = "zxart";

  // indices of parts in path
  private static final int POS_CATEGORY = 0;
  private static final int POS_AUTHOR_NICK = 1;
  private static final int POS_AUTHOR_YEAR = 2;
  private static final int POS_AUTHOR_TRACK = 3;
  private static final int POS_PARTY_YEAR = 1;
  private static final int POS_PARTY_NAME = 2;
  private static final int POS_PARTY_COMPO = 3;
  private static final int POS_PARTY_TRACK = 4;
  private static final int POS_TOP_TRACK = 1;

  private static final String PARAM_AUTHOR_ID = "author";
  private static final String PARAM_PARTY_ID = "party";
  private static final String PARAM_TRACK_ID = "track";

  public static final String CATEGORY_AUTHORS = "Authors";
  public static final String CATEGORY_PARTIES = "Parties";
  public static final String CATEGORY_TOP = "Top";

  private static final String UNKNOWN_YEAR = "unknown";

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
    return forCategory(CATEGORY_AUTHORS)
            .appendPath(author.getNickname())
            .appendQueryParameter(PARAM_AUTHOR_ID, Integer.toString(author.getId()));
  }

  public static Uri.Builder forAuthor(Author author, int year) {
    return forCategory(CATEGORY_AUTHORS)
            .appendPath(author.getNickname())
            .appendPath(year != 0 ? Integer.toString(year) : UNKNOWN_YEAR)
            .appendQueryParameter(PARAM_AUTHOR_ID, Integer.toString(author.getId()));
  }

  @Nullable
  public static Author findAuthor(Uri uri, List<String> path) {
    if (path.size() > POS_AUTHOR_NICK) {
      final String nick = path.get(POS_AUTHOR_NICK);
      final String id = uri.getQueryParameter(PARAM_AUTHOR_ID);
      if (nick != null && id != null) {
        return new Author(Integer.parseInt(id), nick, ""/*fake*/);
      }
    }
    return null;
  }

  @Nullable
  public static Integer findAuthorYear(Uri uri, List<String> path) {
    if (path.size() > POS_AUTHOR_YEAR) {
      final String year = path.get(POS_AUTHOR_YEAR);
      try {
        if (!UNKNOWN_YEAR.equals(year)) {
          return Integer.valueOf(year);
        }
      } catch (NumberFormatException e) {
        return 0;
      }
      return 0;
    } else {
      return null;
    }
  }

  // Parties
  public static Uri.Builder forPartiesYear(int year) {
    return forCategory(CATEGORY_PARTIES)
            .appendPath(Integer.toString(year));
  }

  @Nullable
  public static Integer findPartiesYear(Uri uri, List<String> path) {
    if (path.size() > POS_PARTY_YEAR) {
      return Integer.valueOf(path.get(POS_PARTY_YEAR));
    } else {
      return null;
    }
  }

  public static Uri.Builder forParty(Party party) {
    return forPartiesYear(party.getYear())
            .appendPath(party.getName())
            .appendQueryParameter(PARAM_PARTY_ID, Integer.toString(party.getId()));
  }

  @Nullable
  public static Party findParty(Uri uri, List<String> path) {
    final String id = uri.getQueryParameter(PARAM_PARTY_ID);
    if (id != null && path.size() > POS_PARTY_NAME) {
      final String year = path.get(POS_PARTY_YEAR);
      final String name = path.get(POS_PARTY_NAME);
      return new Party(Integer.parseInt(id), name, Integer.parseInt(year));
    } else {
      return null;
    }
  }

  public static Uri.Builder forPartyCompo(Party party, String compo) {
    return forParty(party)
            .appendPath(compo);
  }

  @Nullable
  public static String findPartyCompo(Uri uri, List<String> path) {
    if (path.size() > POS_PARTY_COMPO) {
      return path.get(POS_PARTY_COMPO);
    } else {
      return null;
    }
  }

  // Tracks
  public static Uri.Builder forTrack(Uri.Builder parent, Track track) {
    return parent
            .appendPath(track.getFilename())
            .appendQueryParameter(PARAM_TRACK_ID, Integer.toString(track.getId()));
  }

  @Nullable
  public static Track findAuthorTrack(Uri uri, List<String> path) {
    final String id = uri.getQueryParameter(PARAM_TRACK_ID);
    if (id != null && path.size() > POS_AUTHOR_TRACK) {
      final String filename = path.get(POS_AUTHOR_TRACK);
      return new Track(Integer.parseInt(id), filename,
              //fake
              "", "", "", 0, "", 0);
    } else {
      return null;
    }
  }

  @Nullable
  public static Track findPartyTrack(Uri uri, List<String> path) {
    final String id = uri.getQueryParameter(PARAM_TRACK_ID);
    if (id != null && path.size() > POS_PARTY_TRACK) {
      final String filename = path.get(POS_PARTY_TRACK);
      return new Track(Integer.parseInt(id), filename,
              //fake
              "", "", "", 0, "", 0);
    } else {
      return null;
    }
  }

  @Nullable
  public static Track findTopTrack(Uri uri, List<String> path) {
    final String id = uri.getQueryParameter(PARAM_TRACK_ID);
    if (id != null && path.size() > POS_TOP_TRACK) {
      final String filename = path.get(POS_TOP_TRACK);
      return new Track(Integer.parseInt(id), filename,
              //fake
              "", "", "", 0, "", 0);
    } else {
      return null;
    }
  }
}
