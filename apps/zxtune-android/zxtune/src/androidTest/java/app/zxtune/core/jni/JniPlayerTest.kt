package app.zxtune.core.jni

import app.zxtune.TimeStamp
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Test

private val BEGINNING = TimeStamp.fromMilliseconds(0)

class JniPlayerWrongHandleTest {

    private val underTest = JniPlayer(-1)

    init {
        JniApi.loadLibraryForTest()
    }

    @Test(expected = IllegalArgumentException::class)
    fun testSetStringProperty() = underTest.setProperty("", "")

    @Test(expected = IllegalArgumentException::class)
    fun testSetLongProperty() = underTest.setProperty("", 0L)

    @Test
    fun testSetPosition() {
        underTest.position = BEGINNING
    }

    @Test
    fun testGetPosition() {
        assertEquals(BEGINNING, underTest.position)
    }

    @Test
    fun testGetPerformance() {
        assertEquals(0L, underTest.performance.toLong())
    }

    @Test
    fun testGetProgress() {
        assertEquals(0L, underTest.progress.toLong())
    }

    @Test
    fun testAnalyze() {
        assertEquals(0L, underTest.analyze(ByteArray(1)).toLong())
    }

    @Test
    fun testRender() {
        assertFalse(underTest.render(ShortArray(1)))
    }

    @Test
    fun testRelease() = underTest.release()
}
