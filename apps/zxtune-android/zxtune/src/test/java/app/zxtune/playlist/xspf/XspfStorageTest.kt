package app.zxtune.playlist.xspf

import android.database.MatrixCursor
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner
import java.io.File

@RunWith(RobolectricTestRunner::class)
class XspfStorageTest {

    private lateinit var root: File
    private lateinit var underTest: XspfStorage

    @Before
    fun setUp() {
        root = File(System.getProperty("java.io.tmpdir", "."), toString())
        underTest = XspfStorage(root)
        assertEquals(false, root.exists())
    }

    @After
    fun tearDown() {
        root.deleteRecursively()
    }

    @Test
    fun `empty root not created`() {
        assertEquals(0, underTest.enumeratePlaylists().size)
    }

    @Test
    fun `delayed root creating every time`() {
        val cursor = MatrixCursor(arrayOf())
        underTest.createPlaylist("1", cursor)
        assertEquals(true, root.exists())
        assertEquals(true, File(root, "1.xspf").exists())
        assertEquals(arrayListOf("1"), underTest.enumeratePlaylists())
        assertEquals("${root.absolutePath}/1.xspf", underTest.findPlaylistPath("1"))
        root.deleteRecursively()

        assertEquals(false, root.exists())
        assertEquals(null, underTest.findPlaylistPath("2"))
        underTest.createPlaylist("2", cursor)
        assertEquals(true, root.exists())
        assertEquals(true, File(root, "2.xspf").exists())
        assertEquals(arrayListOf("2"), underTest.enumeratePlaylists())
        assertEquals("${root.absolutePath}/2.xspf", underTest.findPlaylistPath("2"))
        underTest.createPlaylist("3", cursor)
        assertEquals(true, root.exists())
        assertEquals(true, File(root, "3.xspf").exists())
        assertEquals(arrayListOf("2", "3"), underTest.enumeratePlaylists())
        assertEquals("${root.absolutePath}/3.xspf", underTest.findPlaylistPath("3"))
    }

    @Test
    fun `unsupported files ignored`() {
        root.mkdirs()
        makeFile("unsupported")
        makeFile("supported.xspf")
        makeFile("case sensitive.XSPF")
    }

    private fun makeFile(name : String) = File(root, name).apply {
        createNewFile()
    }.also {
        assertEquals(true, it.isFile && it.exists())
    }
}
