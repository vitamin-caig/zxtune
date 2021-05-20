package app.zxtune.fs.http

import android.net.Uri
import android.os.Build
import app.zxtune.BuildConfig
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import java.io.IOException
import java.io.InputStream

@RunWith(RobolectricTestRunner::class)
class ProviderTest {

    private var provider = HttpProviderFactory.createTestProvider()

    @Test
    fun `test static object`() {
        val uri =
            Uri.parse("http://nsf.joshw.info/n/North%20%26%20South%20(1990-09-21)(Kemco)[NES].7z")
        with(provider.getObject(uri)) {
            assertEquals(uri, this.uri)
            assertEquals(10855, contentLength!!)
            //$ date -u -d 'Sun, 14 Jul 2013 17:32:52 GMT' +%s
            //1373823172
            assertEquals(1373823172, lastModified!!.toSeconds())
            testStream(
                input, 10855, byteArrayOf(0x37, 0x7a, 0xbc, 0xaf, 0x27, 0x1c, 0x00, 0x03),
                byteArrayOf(0x30, 0x2e, 0x39, 0x62, 0x65, 0x74, 0x61)
            )
        }
    }

    @Test
    fun `test redirected object`() =
        with(provider.getObject(Uri.parse("http://amp.dascene.net/downmod.php?index=150946"))) {
            assertEquals(
                "http://amp.dascene.net/modules/E/Estrayk/MOD.%28Starquake%29broken%20heart.gz",
                uri.toString()
            )
            assertEquals(182985, contentLength!!)
            //$ date -u -d 'Sat, 17 Nov 2018 23:04:27 GMT' +%s
            //1542495867
            assertEquals(1542495867, lastModified!!.toSeconds())
            testStream(
                input, 182985,
                byteArrayOf(0x1f, 0x8b, 0x08, 0x08, 0x7b, 0x9e, 0xf0, 0x5b),
                byteArrayOf(0x03, 0xa7, 0x9f, 0x12, 0x99, 0x24, 0xb3, 0x04, 0x00)
            )
        }

    @Test
    fun `test dynamic object`() {
        val uri = Uri.parse("https://storage.zxtune.ru/check")
        with(provider.getObject(uri)) {
            assertEquals(uri, this.uri)
            assertNull(contentLength)
            assertNull(lastModified)
            testStream(
                input, 0x3a,
                byteArrayOf(0x7b, 0x22, 0x69, 0x70, 0x22, 0x3a, 0x22),
                byteArrayOf(0x22, 0x7d, 0x0a)
            )
        }
    }

    @Test(expected = IOException::class)
    fun `test unavailable host`() {
        provider.getObject(Uri.parse("http://invalid.zxtune.ru/document"))
    }

    @Test
    fun `test unavailable file`() {
        try {
            provider.getObject(Uri.parse("http://nsf.joshw.info/ne/"))
            fail("Should not create object")
        } catch (e: IOException) {
            assertNotNull("Thrown exception", e)
            assertEquals(
                "Unexpected code 404 (Not Found) for HEAD request to http://nsf.joshw.info/ne/",
                e.message
            )
        }
    }

    @Test
    fun `test big file`() {
        val uri =
            Uri.parse("${BuildConfig.CDN_ROOT}/browse/joshw/pc/t/Tekken 7 (2017-06-02)(-)(Bandai Namco)[PC].7z")
        with(provider.getObject(uri)) {
            assertEquals(uri, this.uri)
            if (Build.VERSION.SDK_INT >= 24) {
                assertEquals(3683414322L, contentLength!!)
            } else {
                assertNull(contentLength)
            }
            /*testStream(
                input, 3683414322L,
                byteArrayOf(0x37, 0x7a, 0xbc, 0xaf, 0x27, 0x1c, 0x00, 0x04),
                byteArrayOf(0x0a, 0x01, 0xfd, 0x94, 0xa6, 0x6e, 0x00, 0x00)
            );*/
        }
    }
}

private fun byteArrayOf(vararg data: Int) = ByteArray(data.size) { idx ->
    data[idx].toByte()
}

private fun testStream(input: InputStream, size: Long, head: ByteArray, tail: ByteArray) =
    input.use { stream ->
        with(ByteArray(head.size)) {
            assertEquals(head.size, stream.read(this))
            assertArrayEquals(head, this)
        }
        var toSkip = size - head.size - tail.size
        while (toSkip != 0L) {
            toSkip -= stream.skip(toSkip)
        }
        with(ByteArray(tail.size)) {
            assertEquals(tail.size, stream.read(this))
            assertArrayEquals(tail, this)
            assertEquals(-1, stream.read())
        }
    }
