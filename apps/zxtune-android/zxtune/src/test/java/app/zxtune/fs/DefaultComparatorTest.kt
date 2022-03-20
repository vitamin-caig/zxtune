package app.zxtune.fs

import org.junit.Assert.assertEquals
import org.junit.Test

class DefaultComparatorTest {

    @Test
    fun `test letters`() = assertSorted(
        "",
        "ABCD",
        "abcd",
        "ABCDE",
        "abcde",
        "CDEF",
        "cdef",
        "CDEFG",
        "cdefg",
    )

    @Test
    fun `test numeric`() = assertSorted(
        "00",
        "0",
        "001",
        "01",
        "1",
        "05",
        "5",
        "0010",
        "010",
        "10",
        "0100",
        "100",
    )

    @Test
    fun `test numeric with prefix`() = assertSorted(
        "prefix00",
        "prefix0",
        "prefix001",
        "prefix01",
        "prefix1",
        "prefix05",
        "prefix5",
        "prefix0010",
        "prefix010",
        "prefix10",
        "prefix0100",
        "prefix100",
    )

    @Test
    fun `test numeric with suffix`() = assertSorted(
        "00suffix",
        "0suffix",
        "001suffix",
        "01suffix",
        "1suffix",
        "05suffix",
        "5suffix",
        "0010suffix",
        "010suffix",
        "10suffix",
        "0100suffix",
        "100suffix",
    )

    @Test
    fun `test numeric with prefix and suffix`() = assertSorted(
        "prefix00suffix",
        "prefix0suffix",
        "prefix001suffix",
        "prefix01suffix",
        "prefix1suffix",
        "prefix05suffix",
        "prefix5suffix",
        "prefix0010suffix",
        "prefix010suffix",
        "prefix10suffix",
        "prefix0100suffix",
        "prefix100suffix",
    )

    @Test
    fun `test border cases`() = assertSorted(
        "a",
        "ab",
        "ab00",
        "ab0",
        "ab00cd",
        "ab0cd",
        "ab01",
        "cd",
    )

    @Test
    fun `test big integers`() = assertSorted(
        "359032080783004",
        "15747738266091545163",
        "23472943879812349871293841237687263750123984102341237461235123",
    )

    @Test
    fun `test casing`() = assertSorted(
        "cap",
        "Capital.2016",
        "capital.2015",
        "Datum",
    )
}

private fun assertSorted(vararg items: String) {
    for (i in items.indices) {
        items[i].let { lh ->
            for (j in i + 1 until items.size) {
                assertOrdered(lh, items[j])
            }
        }
    }
}

private fun assertOrdered(lh: String, rh: String) {
    assertEquals("$lh < $rh", -1, DefaultComparator.compare(lh, rh))
    assertEquals("$rh > $lh", 1, DefaultComparator.compare(rh, lh))
    assertEquals("$lh == $lh", 0, DefaultComparator.compare(lh, lh))
    assertEquals("$rh == $rh", 0, DefaultComparator.compare(rh, rh))
}
