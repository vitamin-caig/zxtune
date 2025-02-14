package app.zxtune.io

import app.zxtune.assertThrows
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.AdditionalMatchers.gt
import org.mockito.kotlin.argThat
import org.mockito.kotlin.mock
import org.mockito.kotlin.times
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner
import java.io.File
import java.io.IOException

@RunWith(RobolectricTestRunner::class)
class TorrentFileTest {
    private lateinit var filesVisitor: (String, Long) -> Unit

    @Before
    fun setUp() {
        filesVisitor = mock {}
    }

    @After
    fun tearDown() = verifyNoMoreInteractions(filesVisitor)

    @Test
    fun `test multifile torrent`() {
        TorrentFile.parse(getTorrent("Arcadia_Legends")).forEachFile(filesVisitor)
        verify(filesVisitor).invoke("Arcadia Legends/Read Me.txt", 11430)
        verify(filesVisitor).invoke("Arcadia Legends/Artwork/Front embed.png", 781733)
        verify(filesVisitor).invoke(
            "Arcadia Legends/MP3/Disc 4 - Bonus WIPs/B-01 nokbient - Ramirez's Theme (WIP 2).mp3",
            5291010
        )
        verify(filesVisitor, times(114)).invoke(argThat { startsWith("Arcadia Legends/") }, gt(0L))
        verify(filesVisitor, times(47)).invoke(
            argThat { startsWith("Arcadia Legends/MP3") }, gt(0L)
        )
    }

    @Test
    fun `test singlefile torrent`() {
        TorrentFile.parse(getTorrent("ubuntu.iso")).forEachFile(filesVisitor)
        verify(filesVisitor).invoke("ubuntu-24.10-desktop-amd64.iso", 5665497088L)
    }

    @Test
    fun `test integers`() {
        assertEquals(0, TorrentFile.parse("i0e".encodeToByteArray()).node.int)
        assertEquals(-1, TorrentFile.parse("i-1e".encodeToByteArray()).node.int)
    }

    @Test
    fun `test invalid torrents`() {
        for (case in arrayOf(
            "", "abc", "123", "4:cut", "i", "it", "ie", "i-e", "i-0e", "i00e", "i123", "l", "d"
        )) {
            assertThrows<IllegalArgumentException> {
                TorrentFile.parse(case.encodeToByteArray())
            }
        }
        for (case in arrayOf(
            "3:str", "i10e", "le", "de", "detail", "d4:infodee", "d4:infod4:name4:pathee"
        )) {
            with(TorrentFile.parse(case.encodeToByteArray())) {
                assertThrows<IllegalArgumentException> { forEachFile(filesVisitor) }
            }
        }
    }

    @Test
    fun `calgary test`() {
        File("torrents").listFiles({ _, name -> name.endsWith(".torrent") })?.forEach { f ->
            try {
                TorrentFile.parse(f.readBytes()).forEachFile { _, _ -> }
            } catch (e: Exception) {
                throw IOException(f.absolutePath, e)
            }
        }
    }
}

private fun getTorrent(name: String) =
    requireNotNull(TorrentFileTest::class.java.classLoader).getResourceAsStream("torrents/${name}.torrent")
        .use {
            it.readBytes()
        }
