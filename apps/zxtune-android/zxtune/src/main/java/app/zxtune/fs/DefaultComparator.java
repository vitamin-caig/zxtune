/**
 * @file
 * @brief
 * @author vitamin.caig@gmail.com
 */

package app.zxtune.fs;

import java.util.Comparator;

public class DefaultComparator implements Comparator<VfsObject> {

  @Override
  public int compare(VfsObject lh, VfsObject rh) {
    return compare(lh.getName(), rh.getName());
  }

  //allow access for test
  static int compare(String lh, String rh) {
    return compareAlphaNumeric(lh, lh.length(), rh, rh.length());
  }

  private static int compareAlphaNumeric(String lh, int lhSize, String rh, int rhSize) {
    int compareResult = 0;
    int caseSensitiveCompareResult = 0;
    for (int pos = 0; ; ++pos) {
      final boolean lhEnd = pos >= lhSize;
      final boolean rhEnd = pos >= rhSize;
      if (lhEnd || rhEnd) {
        if (compareResult != 0) {
          return compareResult;
        } else if (lhEnd && rhEnd) {
          return caseSensitiveCompareResult;
        } else if (lhEnd) {
          return -1;
        } else {
          return 1;
        }
      }
      final char lhSym = lh.charAt(pos);
      final char rhSym = rh.charAt(pos);

      if (isDigit(lhSym) && isDigit(rhSym)) {
        return compareNumericPrefix(lh.substring(pos), lhSize - pos, rh.substring(pos), rhSize - pos);
      } else if (lhSym != rhSym && compareResult == 0) {
        compareResult = compare(Character.toUpperCase(lhSym), Character.toUpperCase(rhSym));
        if (compareResult == 0 && caseSensitiveCompareResult == 0) {
          caseSensitiveCompareResult = compare(lhSym, rhSym);
        }
      }
    }
  }

  private static int compareNumericPrefix(String lh, int lhSize, String rh, int rhSize) {
    final int lhZeroes = countZeroes(lh, lhSize);
    final int rhZeroes = countZeroes(rh, rhSize);
    if (lhZeroes == 0 && rhZeroes == 0) {
      return compareNumericNoZeroPrefix(lh, lhSize, rh, rhSize);
    } else {
      final int suffixCompareResult = compareNumericNoZeroPrefix(lh.substring(lhZeroes), lhSize - lhZeroes, rh.substring(rhZeroes), rhSize - rhZeroes);
      return suffixCompareResult != 0
              ? suffixCompareResult
              : -compare(lhZeroes, rhZeroes);
    }
  }

  private static int countZeroes(String str, int size) {
    for (int res = 0; res < size; ++res) {
      if (str.charAt(res) != '0') {
        return res;
      }
    }
    return size;
  }

  private static int compareNumericNoZeroPrefix(String lh, int lhSize, String rh, int rhSize) {
    int compareResult = 0;
    for (int pos = 0; ; ++pos) {
      final boolean lhEnd = pos >= lhSize;
      final boolean rhEnd = pos >= rhSize;
      if (lhEnd && rhEnd) {
        return compareResult;
      } else if (lhEnd) {
        return -1;
      } else if (rhEnd) {
        return 1;
      }
      final char lhSym = lh.charAt(pos);
      final char rhSym = rh.charAt(pos);
      final boolean lhDigit = isDigit(lhSym);
      final boolean rhDigit = isDigit(rhSym);
      if (lhDigit && rhDigit) {
        if (compareResult == 0) {
          compareResult = compare(lhSym, rhSym);
        }
      } else if (rhDigit) {
        return -1;
      } else if (lhDigit) {
        return 1;
      } else if (compareResult != 0) {
        return compareResult;
      } else {
        return compareAlphaNumeric(lh.substring(pos), lhSize - pos, rh.substring(pos), rhSize - pos);
      }
    }
  }

  private static boolean isDigit(char c) {
    return c >= '0' && c <= '9';
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

  public static Comparator<VfsObject> instance() {
    return Holder.INSTANCE;
  }

  //onDemand holder idiom
  private static class Holder {
    public static final DefaultComparator INSTANCE = new DefaultComparator();
  }
}
