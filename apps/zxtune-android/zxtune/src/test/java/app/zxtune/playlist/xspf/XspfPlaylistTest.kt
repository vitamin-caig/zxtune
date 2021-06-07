package app.zxtune.playlist.xspf

import android.net.Uri
import app.zxtune.TimeStamp
import app.zxtune.core.Identifier
import app.zxtune.io.Io.copy
import app.zxtune.playlist.Item
import app.zxtune.playlist.xspf.XspfIterator.parse
import org.junit.Assert.*
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import java.io.ByteArrayOutputStream
import java.io.IOException

/**
 * TODO:
 * - item's properties
 */
@RunWith(RobolectricTestRunner::class)
class XspfPlaylistTest {
    @Test(expected = IOException::class)
    fun `test parse empty string`() {
        parse(getPlaylistResource("zero"))
        fail("Should not be called")
    }

    @Test
    fun `test parse empty`() = parse(getPlaylistResource("empty")).let {
        assertTrue(it.isEmpty())
    }

    @Test
    fun `test parse minimal`() = parse(getPlaylistResource("minimal")).let { list ->
        assertEquals(1, list.size.toLong())
        with (list[0]) {
            assertEquals("file:/folder/escaped%20path", location)
            assertEquals(TimeStamp.EMPTY, duration)
            assertEquals("", title)
            assertEquals("", author)
        }
    }

    @Test
    fun `test parse mobile v1`() = parse(getPlaylistResource("mobile_v1")).let { list ->
        assertEquals(3, list.size.toLong())
        with(list[0]) {
            assertEquals("file:/folder/only%20duration", location)
            assertEquals(TimeStamp.fromMilliseconds(123456), duration)
            assertEquals("", title)
            assertEquals("", author)
        }
        with(list[1]) {
            assertEquals(
                "joshw:/some/%D1%84%D0%B0%D0%B9%D0%BB.7z#%D0%BF%D0%BE%D0%B4%D0%BF%D0%B0%D0%BF%D0%BA%D0%B0%2Fsubfile.mp3",
                location
            )
            assertEquals(TimeStamp.fromMilliseconds(2345), duration)
            assertEquals("Название", title)
            assertEquals("Author Unknown", author)
        }
        with(list[2]) {
            assertEquals("hvsc:/GAMES/file.sid#%233", location)
            assertEquals(TimeStamp.fromMilliseconds(6789), duration)
            assertEquals("Escaped%21", title)
            assertEquals("Me%)", author)
        }
    }

    @Test
    fun `test parse mobile v2`() = parse(getPlaylistResource("mobile_v2")).let { list ->
        assertEquals(3, list.size.toLong())
        with(list[0]) {
            assertEquals("file:/folder/only%20duration", location)
            assertEquals(TimeStamp.fromMilliseconds(123456), duration)
            assertEquals("", title)
            assertEquals("", author)
        }
        with(list[1]) {
            assertEquals(
                "joshw:/some/%D1%84%D0%B0%D0%B9%D0%BB.7z#%D0%BF%D0%BE%D0%B4%D0%BF%D0%B0%D0%BF%D0%BA%D0%B0%2Fsubfile.mp3",
                location
            )
            assertEquals(TimeStamp.fromMilliseconds(2345), duration)
            assertEquals("Название", title)
            assertEquals("Author Unknown", author)
        }
        with(list[2]) {
            assertEquals("hvsc:/GAMES/file.sid#%233", location)
            assertEquals(TimeStamp.fromMilliseconds(6789), duration)
            assertEquals("Escaped%21", title)
            assertEquals("Me%)", author)
        }
    }

    @Test
    fun `test parse desktop`() = parse(getPlaylistResource("desktop")).let { list ->
        assertEquals(2, list.size.toLong())
        with(list[0]) {
            assertEquals("chiptunes/RP2A0X/nsfe/SuperContra.nsfe#%232", location)
            assertEquals(TimeStamp.fromMilliseconds(111300), duration)
            assertEquals("Stage 1 - Lightning and Grenades", title)
            assertEquals("H.Maezawa", author)
        }
        with(list[1]) {
            assertEquals("../chiptunes/DAC/ZX/dst/Untitled.dst", location)
            assertEquals(TimeStamp.fromMilliseconds(186560), duration)
            assertEquals("Untitled2(c)&(p)Cj.Le0'99aug", title)
            assertEquals("", author)
        }
    }

    @Test
    fun `test create empty`() = with(ByteArrayOutputStream(1024)) {
        Builder(this).apply {
            writePlaylistProperties("Пустой", null)
        }.finish()
        assertEquals(getPlaylistReference("empty"), toString())
    }

    @Test
    fun `test create full`() = with(ByteArrayOutputStream(1024)) {
        Builder(this).apply {
            writePlaylistProperties("Полный", 3)
            writeTrack(makeItem("file:/folder/only duration", "", "", 123456))
            writeTrack(
                makeItem(
                    "joshw:/some/файл.7z#подпапка/subfile.mp3",
                    "Название",
                    "Author Unknown",
                    2345
                )
            )
            writeTrack(makeItem("hvsc:/GAMES/file.sid##3", "Escaped%21", "Me%)", 6789))
        }.finish()
        assertEquals(getPlaylistReference("mobile_v2"), toString())
    }
}

private fun getPlaylistResource(name: String) =
    XspfPlaylistTest::class.java.classLoader!!.getResourceAsStream("playlists/$name.xspf")

private fun getPlaylistReference(name: String) = with(ByteArrayOutputStream()) {
    copy(getPlaylistResource(name), this)
    toString()
}

private fun makeItem(id: String, title: String, author: String, duration: Long) =
    Item(createIdentifier(id), title, author, TimeStamp.fromMilliseconds(duration))

// To force encoding
private fun createIdentifier(decoded: String) = with(Uri.parse(decoded)) {
    Identifier(Uri.Builder().scheme(scheme).path(path).fragment(fragment).build())
}
