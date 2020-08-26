/**
 * @file
 * @brief Time stamp value type
 * @author vitamin.caig@gmail.com
 */

package app.zxtune;

import java.util.Locale;
import java.util.concurrent.TimeUnit;

public final class TimeStamp implements Comparable<TimeStamp> {

  private static final TimeUnit UNIT = TimeUnit.MILLISECONDS;
  private final long value;

  private TimeStamp(long val, TimeUnit unit) {
    this.value = UNIT.convert(val, unit);
  }

  public static final TimeStamp EMPTY = createFrom(0, UNIT);

  public static TimeStamp createFrom(long val, TimeUnit unit) {
    return new TimeStamp(val, unit);
  }

  public final long convertTo(TimeUnit unit) {
    return unit.convert(value, UNIT);
  }

  @Override
  public String toString() {
    final int totalSec = (int) UNIT.toSeconds(value);
    final int totalMin = totalSec / 60;
    final int totalHour = totalMin / 60;
    final int min = totalMin % 60;
    final int sec = totalSec % 60;
    return totalHour != 0 ? String.format(Locale.US, "%d:%02d:%02d", totalHour, min, sec) : String
        .format(Locale.US, "%d:%02d", min, sec);
  }

  @Override
  public int hashCode() {
    return (int) (value ^ (value >>> 32));
  }

  @Override
  public boolean equals(Object rh) {
    return this == rh || (rh != null && rh.getClass() == getClass() && value == ((TimeStamp) rh).value);
  }

  @Override
  public int compareTo(TimeStamp rh) {
    return value == rh.value ? 0 : (value < rh.value ? -1 : +1);
  }

  public final long divides(TimeStamp rh) {
    return value / rh.value;
  }

  public final TimeStamp multiplies(long count) {
    return new TimeStamp(value * count, UNIT);
  }
}
