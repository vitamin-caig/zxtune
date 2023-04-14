package app.zxtune.ui.utils

import org.junit.Assert.*
import org.junit.Test

class FilteredListStateTest {

    private val filter1 = "filter"
    private val filter2 = filter1 + "2"

    private val entry1 = "unmatched"
    private val entry2 = "${filter1}suffix"
    private val entry3 = "${filter2}suffix"

    @Test
    fun `main logic test`() {
        val initial = FilteredListState<String> { item, filter ->
            item.startsWith(filter)
        }.apply {
            assertEquals("", filter)
            entries.run {
                assertNull(this as? MutableList<String>)
                assertEquals(0, size)
            }
        }
        val filled2 = initial.withContent(mutableListOf(entry1, entry2)).apply {
            assertEquals("", filter)
            entries.run {
                assertNotNull(this as? MutableList<String>)
                assertEquals(2, size)
                assertSame(entry1, get(0))
                assertSame(entry2, get(1))
            }
        }
        filled2.withContent(arrayListOf()).run {
            assertEquals("", filter)
            entries.run {
                assertNotNull(this as? MutableList<String>)
                assertEquals(0, size)
            }
        }
        filled2.withFilter(" ").run {
            assertEquals("", filter)
            entries.run {
                assertNotNull(this as? MutableList<String>)
                assertEquals(2, size)
                assertSame(entry1, get(0))
                assertSame(entry2, get(1))
            }
        }
        val filtered3 = filled2.withFilter(filter1).apply {
            assertEquals(filter1, filter)
            entries.run {
                assertNull(this as? MutableList<String>)
                assertEquals(1, size)
                assertSame(entry2, get(0))
            }
        }
        val filtered4 = filtered3.withContent(arrayListOf()).apply {
            assertEquals(filter1, filter)
            entries.run {
                assertNull(this as? MutableList<String>)
                assertEquals(0, size)
            }
        }
        val filtered5 = filtered4.withContent(arrayListOf(entry1, entry2, entry3)).apply {
            assertEquals(filter1, filter)
            entries.run {
                assertNull(this as? MutableList<String>)
                assertEquals(2, size)
                assertSame(entry2, get(0))
                assertSame(entry3, get(1))
            }
        }
        filtered5.withFilter(filter2).run {
            assertEquals(filter2, filter)
            entries.run {
                assertNull(this as? MutableList<String>)
                assertEquals(1, size)
                assertSame(entry3, get(0))
            }
        }
        filtered5.withFilter("").run {
            assertEquals("", filter)
            entries.run {
                assertNotNull(this as? MutableList<String>)
                assertEquals(3, size)
                assertSame(entry1, get(0))
                assertSame(entry2, get(1))
                assertSame(entry3, get(2))
            }
        }
    }
}
