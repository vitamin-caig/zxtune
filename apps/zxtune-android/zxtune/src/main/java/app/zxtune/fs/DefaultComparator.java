/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune.fs;

import java.util.Comparator;

/*
 * In case of equal prefixes before digits sequence, use number-ordered comparing.
 * Else use case-insensitive comparing
 */
public class DefaultComparator implements Comparator<VfsObject> {

  @Override
  public int compare(VfsObject lh, VfsObject rh) {
    return compare(lh.getName(), rh.getName());
  }

  private static int compare(String lh, String rh) {
    final int lhSize = lh.length();
    final int rhSize = rh.length();
    for (int pos = 0; ; ++pos) {
      final boolean lhEnd = pos >= lhSize;
      final boolean rhEnd = pos >= rhSize;
      if (lhEnd && rhEnd) {
        return 0;
      } else if (lhEnd) {
        return -1;
      } else if (rhEnd) {
        return 1;
      }
      final char lhSym = lh.charAt(pos);
      final char rhSym = rh.charAt(pos);
      if (isDigit(lhSym) && isDigit(rhSym)) {
        return compareNumeric(lh.substring(pos), rh.substring(pos));
      } else if (lhSym != rhSym) {
        break;
      }
    }
    return compareDefault(lh, rh);
  }

  private static boolean isDigit(char c) {
    return c >= '0' && c <= '9';
  }

  private static int compareDefault(String lh, String rh) {
    return String.CASE_INSENSITIVE_ORDER.compare(lh, rh);
  }

  private static int compareNumeric(String lh, String rh) {
    final Integer lhNumber = parseNumber(lh);
    if (lhNumber != null) {
      final Integer rhNumber = parseNumber(rh);
      if (rhNumber != null) {
        if (!lhNumber.equals(rhNumber)) {
          return compare(lhNumber, rhNumber);
        }
      }
    }
    return compareDefault(lh, rh);
  }

  private static int compare(int lh, int rh) {
    if (lh < rh) {
      return -1;
    } else if (lh > rh) {
      return 1;
    } else {
      return 0;
    }
  }

  private static Integer parseNumber(String str) {
    final int size = str.length();
    for (int pos = 0; pos < size; ++pos) {
      if (!isDigit(str.charAt(pos))) {
        return null;
      }
    }
    return Integer.decode(str);
  }

  public static Comparator<VfsObject> instance() {
    return Holder.INSTANCE;
  }
  
  //onDemand holder idiom
  private static class Holder {
    public static final DefaultComparator INSTANCE = new DefaultComparator();
  }
}
