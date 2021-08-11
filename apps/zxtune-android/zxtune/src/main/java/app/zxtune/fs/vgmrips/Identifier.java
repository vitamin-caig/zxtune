/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs.vgmrips;

import android.net.Uri;

import androidx.annotation.Nullable;

import java.util.List;

import app.zxtune.TimeStamp;

/**
 * Paths:
 * <p>
 * 1) vgmrips:/
 * 2) vgmrips:/Company
 * 3) vgmrips:/Company/${company_name}?group=${company_id}
 * 4) vgmrips:/Composer
 * 5) vgmrips:/Composer/${composer_name}?group=${composer_id}
 * 6) vgmrips:/Chip
 * 7) vgmrips:/Chip/${chip_name}?group=${chip_id}
 * 8) vgmrips:/System
 * 9) vgmrips:/System/${system_name}?group=${system_id}
 * 10) vgmrips://Random
 * <p>
 * X) ${url}&pack=${pack_id}
 */

public class Identifier {

  private static final String SCHEME = "vgmrips";

  private static final int POS_CATEGORY = 0;
  private static final int POS_GROUP_NAME = 1;
  private static final int POS_PACK_NAME = 2;
  private static final int POS_RANDOM_PACK_NAME = 1;

  private static final String PARAM_GROUP = "group";
  private static final String PARAM_PACK = "pack";
  private static final String PARAM_TRACK = "track";

  public static final String CATEGORY_COMPANY = "Company";
  public static final String CATEGORY_COMPOSER = "Composer";
  public static final String CATEGORY_CHIP = "Chip";
  public static final String CATEGORY_SYSTEM = "System";
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

  public static Uri.Builder forGroup(Uri.Builder parent, Group obj) {
    return parent
        .appendPath(obj.getTitle())
        .appendQueryParameter(PARAM_GROUP, obj.getId());
  }

  @Nullable
  public static Group findGroup(Uri uri, List<String> path) {
    if (path.size() > POS_GROUP_NAME) {
      final String id = uri.getQueryParameter(PARAM_GROUP);
      if (id != null) {
        final String name = path.get(POS_GROUP_NAME);
        return new Group(id, name, 0/*fake*/);
      }
    }
    return null;
  }

  public static Uri.Builder forPack(Uri.Builder parent, Pack pack) {
    return parent.appendPath(pack.getTitle()).appendQueryParameter(PARAM_PACK, pack.getId());
  }

  @Nullable
  public static Pack findPack(Uri uri, List<String> path) {
    if (path.size() > POS_PACK_NAME) {
      final String id = uri.getQueryParameter(PARAM_PACK);
      if (id != null) {
        final String name = path.get(POS_PACK_NAME);
        return new Pack(id, name);
      }
    }
    return null;
  }

  @Nullable
  public static Pack findRandomPack(Uri uri, List<String> path) {
    if (path.size() > POS_RANDOM_PACK_NAME) {
      final String id = uri.getQueryParameter(PARAM_PACK);
      if (id != null) {
        final String name = path.get(POS_RANDOM_PACK_NAME);
        return new Pack(id, name);
      }
    }
    return null;
  }

  public static Uri.Builder forTrack(Uri.Builder parent, Track track) {
    return parent.appendPath(track.getTitle()).appendQueryParameter(PARAM_TRACK, track.getLocation());
  }

  @Nullable
  public static Track findTrack(Uri uri, List<String> path) {
    final int elements = path.size();
    if (elements >= 2) {
      final String id = uri.getQueryParameter(PARAM_TRACK);
      if (id != null) {
        final String name = path.get(elements - 1);
        return new Track(0/*fake*/, name, TimeStamp.EMPTY/*fake*/, id);
      }
    }
    return null;
  }
}
