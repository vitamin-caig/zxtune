package app.zxtune

import org.junit.Assert.assertEquals
import org.junit.Test

class TimeStampTest {
    @Test
    fun `test empty`() = with(TimeStamp.EMPTY) {
        assertEquals(0L, toMicroseconds())
        assertEquals("0:00", toString())
    }

    @Test
    fun `test factory and precision`() {
        with(TimeStamp.fromMicroseconds(12345678)) {
            assertEquals(12345000L, toMicroseconds())
        }
        with(TimeStamp.fromMilliseconds(12345678)) {
            assertEquals(12345678L, toMilliseconds())
        }
        with(TimeStamp.fromSeconds(123456)) {
            assertEquals(123456000, toMilliseconds())
        }
    }

    @Test
    fun `test serialize`() {
        with(TimeStamp.fromSeconds(62)) {
            assertEquals("1:02", toString())
        }
        with(TimeStamp.fromSeconds(3723)) {
            assertEquals("1:02:03", toString())
        }
    }

    @Test
    fun `test compare`() {
        val s1 = TimeStamp.fromSeconds(1)
        val ms1000 = TimeStamp.fromMilliseconds(1000)
        val ms1001 = TimeStamp.fromMilliseconds(1001)

        testCompare(s1, s1, 0)
        testCompare(s1, ms1000, 0)
        testCompare(s1, ms1001, -1)
        testCompare(ms1000, ms1000, 0)
        testCompare(ms1000, ms1001, -1)
    }

    @Test
    fun `test multiply`() = with(TimeStamp.fromMilliseconds(33)) {
        assertEquals(99, multiplies(3).toMilliseconds())
    }
}

private fun testCompare(lh: TimeStamp, rh: TimeStamp, expected: Int) {
    val msg = "${lh.toMilliseconds()}ms vs ${rh.toMilliseconds()}ms"
    assertEquals(msg, expected, lh.compareTo(rh))
    assertEquals(msg, -expected, rh.compareTo(lh))
    assertEquals(msg, expected == 0, lh == rh)
    assertEquals(msg, lh == rh, rh == lh)
}
