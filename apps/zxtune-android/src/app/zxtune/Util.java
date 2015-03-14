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
}
