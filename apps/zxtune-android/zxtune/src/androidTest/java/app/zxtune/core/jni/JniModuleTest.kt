package app.zxtune.core.jni

import org.junit.Test
import java.nio.ByteBuffer

class JniModuleWrongHandleTest {

    private val underTest = JniModule(-1)

    init {
        JniApi.loadLibraryForTest()
    }

    @Test(expected = IllegalArgumentException::class)
    fun testGetStringProperty() {
        underTest.getProperty("", "")
    }

    @Test(expected = IllegalArgumentException::class)
    fun testGetLongProperty() {
        underTest.getProperty("", 1L)
    }

    @Test(expected = IllegalArgumentException::class)
    fun testGetAdditionalFiles() {
        underTest.additionalFiles
    }

    @Test(expected = IllegalArgumentException::class)
    fun testResolveAdditionalFiles() {
        underTest.resolveAdditionalFile("", ByteBuffer.allocate(1))
    }

    @Test(expected = IllegalArgumentException::class)
    fun testGetDuration() {
        underTest.duration
    }

    @Test(expected = IllegalArgumentException::class)
    fun testCreatePlayer() {
        underTest.createPlayer(44100)
    }

    @Test
    fun testRelease() = underTest.release()
}
