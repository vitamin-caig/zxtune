package app.zxtune.core.jni

import org.junit.Assert.assertEquals
import org.junit.Test

class JniOptionsTest {

    init {
        JniApi.loadLibraryForTest()
    }

    @Test
    fun testStringProperties() = with(JniOptions.instance()) {
        val name = "string_property"
        val value = "string value"
        assertEquals("", getProperty(name, ""))
        assertEquals("defval", getProperty(name, "defval"))
        setProperty(name, value)
        assertEquals(value, getProperty(name, ""))
        assertEquals(value, getProperty(name, "defval"))
    }

    @Test
    fun testIntegerProperties() = with(JniOptions.instance()) {
        val name = "integer_property"
        val value: Long = 123456
        assertEquals(-1, getProperty(name, -1))
        assertEquals(1000, getProperty(name, 1000))
        setProperty(name, value)
        assertEquals(value, getProperty(name, -1))
        assertEquals(value, getProperty(name, 1000))
    }
}
