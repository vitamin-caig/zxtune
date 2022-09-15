package app.zxtune.sound

import org.junit.After
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import java.io.File

class WaveWriteSamplesTargetTest {

    private lateinit var root: File
    private lateinit var file: File
    private lateinit var underTest: WaveWriteSamplesTarget

    @Before
    fun setUp() {
        root = File(System.getProperty("java.io.tmpdir", "."), toString()).apply {
            require(mkdirs())
        }
        file = File(root, "test.wav")
        underTest = WaveWriteSamplesTarget(file.absolutePath)
    }

    @After
    fun tearDown() {
        root.deleteRecursively()
    }

    @Test
    fun `empty file`() {
        underTest.run {
            start()
            stop()
        }
        file.run {
            assertEquals(44L, length())
            assertArrayEquals(
                byteArrayOf(
                    // signature
                    0x52, 0x49, 0x46, 0x46,
                    // next data size = 36
                    0x24, 0x00, 0x00, 0x00,
                    // signatures
                    0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,
                    // chunk size = 16
                    0x10, 0x00, 0x00, 0x00,
                    // pcm = 1
                    0x01, 0x00,
                    // mono
                    0x01, 0x00,
                    // samplerate = 44100
                    0x44, 0xac.toByte(), 0x00, 0x00,
                    // bytes per sec 44100 * 2
                    0x88.toByte(), 0x58, 0x01, 0x00,
                    // align = 2
                    0x02, 0x00,
                    // bps = 16
                    0x10, 0x00,
                    // signature
                    0x64, 0x61, 0x74, 0x61,
                    // data size = 0
                    0x00, 0x00, 0x00, 0x00
                ), readBytes()
            )
        }
    }

    @Test
    fun `test stereo`() {
        underTest.run {
            start()
            // 7 samples
            writeSamples(
                shortArrayOf(
                    0, 0,
                    1, 1,
                    -1, -1,
                    1, 0,
                    0, -1,
                    32767, 32767,
                    -32768, -32768,
                )
            )
            stop()
        }
        file.run {
            assertEquals(44L + 14, length())
            assertArrayEquals(
                byteArrayOf(
                    // signature
                    0x52, 0x49, 0x46, 0x46,
                    // next data size = 36 + 14
                    0x32, 0x00, 0x00, 0x00,
                    // signatures
                    0x57, 0x41, 0x56, 0x45, 0x66, 0x6d, 0x74, 0x20,
                    // chunk size = 16
                    0x10, 0x00, 0x00, 0x00,
                    // pcm = 1
                    0x01, 0x00,
                    // mono
                    0x01, 0x00,
                    // samplerate = 44100
                    0x44, 0xac.toByte(), 0x00, 0x00,
                    // bytes per sec 44100 * 2
                    0x88.toByte(), 0x58, 0x01, 0x00,
                    // align = 2
                    0x02, 0x00,
                    // bps = 16
                    0x10, 0x00,
                    // signature
                    0x64, 0x61, 0x74, 0x61,
                    // data size = 0
                    0x0e, 0x00, 0x00, 0x00,
                    // data
                    0x00, 0x00, // 0
                    0x01, 0x00, // 1
                    0xff.toByte(), 0xff.toByte(), // -1
                    0x00, 0x00, // 0
                    0x00, 0x00, // 0
                    0xff.toByte(), 0x7f, // 32767
                    0x00, 0x80.toByte(),  // -32768
                ), readBytes()
            )
        }
    }
}
