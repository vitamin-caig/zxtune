/**
 * 
 * @file
 *
 * @brief
 *
 * @author vitamin.caig@gmail.com
 * 
 */

package app.zxtune;

import java.util.Locale;

public class Util {

  //! @return "${left}" or "${right}" or "${left} - ${right}" or "${fallback}"
  public static String formatTrackTitle(String left, String right, String fallback) {
    final boolean noLeft = 0 == left.length();
    final boolean noRight = 0 == right.length();
    if (noLeft && noRight) {
      return fallback;
    } else {
      final StringBuilder result = new StringBuilder();
      result.append(left);
      if (!noLeft && !noRight) {
        result.append(" - ");
      }
      result.append(right);
      return result.toString();
    }
  }

  public static String formatSize(long v) {
    if (v < 1024) {
      return Long.toString(v);
    } else {
      final int z = (63 - Long.numberOfLeadingZeros(v)) / 10;
      return String.format(Locale.US, "%.1f%s", (float) v / (1 << (z * 10)), " KMGTPE".charAt(z));
    }
  }
}
