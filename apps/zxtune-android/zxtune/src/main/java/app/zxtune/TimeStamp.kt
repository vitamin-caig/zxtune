/**
 * @file
 * @brief Time stamp value type
 * @author vitamin.caig@gmail.com
 */
package app.zxtune

import java.util.concurrent.TimeUnit

private val UNIT = TimeUnit.MILLISECONDS

class TimeStamp private constructor(private val value: Long) : Comparable<TimeStamp> {

    private constructor(input: Long, unit: TimeUnit) : this(UNIT.convert(input, unit))

    fun toMicroseconds() = UNIT.toMicros(value)

    fun toMilliseconds() = UNIT.toMillis(value)

    fun toSeconds() = UNIT.toSeconds(value)

    override fun toString() = UNIT.toSeconds(value).let { totalSec ->
        val totalMin = totalSec / 60
        val totalHour = totalMin / 60
        val min = totalMin % 60
        val sec = totalSec % 60
        if (totalHour != 0L)
            "%d:%02d:%02d".format(totalHour, min, sec)
        else
            "%d:%02d".format(min, sec)
    }

    override fun hashCode() = (value xor (value ushr 32)).toInt()

    override fun equals(other: Any?) = value == (other as? TimeStamp)?.value

    override operator fun compareTo(other: TimeStamp) = when {
        value == other.value -> 0
        value < other.value -> -1
        else -> +1
    }

    fun multiplies(count: Long) = TimeStamp(value * count)

    companion object {
        @JvmField
        val EMPTY = TimeStamp(0, UNIT)

        @JvmStatic
        fun fromMicroseconds(value: Long) = TimeStamp(value, TimeUnit.MICROSECONDS)

        @JvmStatic
        fun fromMilliseconds(value: Long) = TimeStamp(value, TimeUnit.MILLISECONDS)

        @JvmStatic
        fun fromSeconds(value: Long) = TimeStamp(value, TimeUnit.SECONDS)

        @JvmStatic
        fun fromDays(value: Long) = TimeStamp(value, TimeUnit.DAYS)
    }
}
