/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.amp;

import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.List;

/**
 * Paths:
 * <p>
 * 1) amp:/
 * 2) amp:/Handle
 * 3) amp:/Handle/${author_letter}
 * 4) amp:/Handle/${author_letter}/${author_name}?author=${author_id}
 * 5) amp:/Country
 * 6) amp:/Country/${country_name}?country=${country_id}
 * 7) amp:/Country/${country_name}/${author_name}?author=${author_id}&country=${country_id}
 * 8) amp:/Group
 * 9) amp:/Group/${group_name}?group=${group_id}
 * 10) amp:/Group/${group_name}/${author_name}?author=${author_id}&group=${group_id}
 */

public class Identifier {

  private static final String SCHEME = "amp";

  private static final int POS_CATEGORY = 0;
  private static final int POS_HANDLE_LETTER = 1;
  private static final int POS_COUNTRY_NAME = 1;
  private static final int POS_GROUP_NAME = 1;
  private static final int POS_AUTHOR_NAME = 2;
  private static final int POS_TRACK_FILENAME = 3;

  private static final String PARAM_COUNTRY = "country";
  private static final String PARAM_GROUP = "group";
  private static final String PARAM_AUTHOR = "author";
  private static final String PARAM_TRACK = "track";

  public static final String CATEGORY_HANDLE = "Handle";
  public static final String CATEGORY_COUNTRY = "Country";
  public static final String CATEGORY_GROUP = "Group";

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

  // Handles
  public static Uri.Builder forHandleLetter(String letter) {
    return forCategory(CATEGORY_HANDLE).appendPath(letter);
  }

  public static boolean isHandleLetter(String letter) {
    return letter.length() == 1 &&
        Character.isLetter(letter.charAt(0)) &&
        Character.isUpperCase(letter.charAt(0));
  }

  @Nullable
  public static String findHandleLetter(Uri uri, List<String> path) {
    if (path.size() > POS_HANDLE_LETTER) {
      final String letter = path.get(POS_HANDLE_LETTER);
      if (Catalog.NON_LETTER_FILTER.equals(letter) || isHandleLetter(letter)) {
        return letter;
      }
    }
    return null;
  }

  // Countries
  public static Uri.Builder forCountry(Country country) {
    return forCategory(CATEGORY_COUNTRY)
        .appendPath(country.getName())
        .appendQueryParameter(PARAM_COUNTRY, String.valueOf(country.getId()));
  }

  @Nullable
  public static Country findCountry(Uri uri, List<String> path) {
    if (path.size() > POS_COUNTRY_NAME) {
      final String id = uri.getQueryParameter(PARAM_COUNTRY);
      if (id != null) {
        final String name = path.get(POS_COUNTRY_NAME);
        return new Country(Integer.valueOf(id), name);
      }
    }
    return null;
  }

  // Groups
  public static Uri.Builder forGroup(Group group) {
    return forCategory(CATEGORY_GROUP)
        .appendPath(group.name)
        .appendQueryParameter(PARAM_GROUP, String.valueOf(group.id));
  }

  @Nullable
  public static Group findGroup(Uri uri, List<String> path) {
    if (path.size() > POS_GROUP_NAME) {
      final String id = uri.getQueryParameter(PARAM_GROUP);
      if (id != null) {
        final String name = path.get(POS_GROUP_NAME);
        return new Group(Integer.valueOf(id), name);
      }
    }
    return null;
  }

  // Authors
  public static Uri.Builder forAuthor(Uri.Builder parent, Author author) {
    return parent.appendPath(author.getHandle()).appendQueryParameter(PARAM_AUTHOR, String.valueOf(author.getId()));
  }

  @Nullable
  public static Author findAuthor(Uri uri, List<String> path) {
    if (path.size() > POS_AUTHOR_NAME) {
      final String id = uri.getQueryParameter(PARAM_AUTHOR);
      if (id != null) {
        final String name = path.get(POS_AUTHOR_NAME);
        return new Author(Integer.valueOf(id), name, ""/*fake*/);
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
    if (path.size() > POS_TRACK_FILENAME) {
      final String id = uri.getQueryParameter(PARAM_TRACK);
      if (id != null) {
        final String name = path.get(POS_TRACK_FILENAME);
        return new Track(Integer.valueOf(id), name, 0/*fake*/);
      }
    }
    return null;
  }
}
