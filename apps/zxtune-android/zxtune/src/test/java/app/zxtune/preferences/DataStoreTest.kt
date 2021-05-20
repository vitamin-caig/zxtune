package app.zxtune.preferences

import android.os.Bundle
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.doReturn
import org.mockito.kotlin.eq
import org.mockito.kotlin.mock
import org.mockito.kotlin.verify
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class DataStoreTest {
    @Test
    fun `test DataStore`() {
        val cache = Bundle().apply {
            putBoolean("none.boolean", true)
            putInt("none.int", 234)
            putLong("none.long", 567)
            putString("none.string", "none")
        }
        val client = mock<ProviderClient>() {
            on { all } doReturn cache
        }
        val store = DataStore(client)

        with(store) {
            assertFalse(getBoolean("zxtune.boolean", false))
            assertTrue(getBoolean("none.boolean", false))
            assertEquals(123, getInt("zxtune.int", 123))
            assertEquals(234, getInt("none.int", 23))
            assertEquals(456, getLong("zxtune.long", 456))
            assertEquals(567, getLong("none.long", 56))
            assertEquals("test", getString("zxtune.string", "test"))
            assertEquals("none", getString("none.string", "default"))
        }

        val bundle = Bundle().apply {
            putBoolean("zxtune.boolean", true)
            putInt("zxtune.int", 12)
            putLong("none.long", 67)
            putString("none.string", "empty")
        }

        with(store) {
            putBoolean("none.boolean", false)
            putInt("none.int", 34)
            putLong("zxtune.long", 56)
            putString("zxtune.string", "stored")
            putBatch(bundle)

            assertEquals(12, getInt("zxtune.int", 0))
            assertEquals(34, getInt("none.int", 0))
            assertEquals(56, getLong("zxtune.long", 0))
            assertEquals(67, getLong("none.long", 0))
            assertEquals("stored", getString("zxtune.string", ""))
            assertEquals("empty", getString("none.string", "default"))
        }
        with(cache) {
            assertEquals(8, size().toLong())
            assertEquals(12, getInt("zxtune.int", 0))
            assertEquals(34, getInt("none.int", 0))
            assertEquals(56, getLong("zxtune.long", 0))
            assertEquals(67, getLong("none.long", 0))
            assertEquals("stored", getString("zxtune.string", ""))
            assertEquals("empty", getString("none.string", "default"))
        }
        verify(client).all
        verify(client).set(eq("none.boolean"), eq(false))
        verify(client).set(eq("none.int"), eq(34))
        verify(client).set(eq("zxtune.long"), eq(56L))
        verify(client).set(eq("zxtune.string"), eq("stored"))
        verify(client).set(eq(bundle))
    }
}
