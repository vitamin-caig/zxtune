package app.zxtune.playlist.xspf

import android.content.ContentResolver
import android.database.MatrixCursor
import android.net.Uri
import androidx.core.net.toFile
import androidx.core.net.toUri
import androidx.documentfile.provider.DocumentFile
import app.zxtune.device.PersistentStorage
import kotlinx.coroutines.test.runTest
import org.junit.After
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.any
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.mock
import org.robolectric.RobolectricTestRunner
import java.io.File
import java.io.FileOutputStream

@RunWith(RobolectricTestRunner::class)
class XspfStorageTest {

    private lateinit var storage: File
    private lateinit var root: File
    private lateinit var resolver: ContentResolver
    private lateinit var underTest: XspfStorage

    @Before
    fun setUp() {
        storage = File(System.getProperty("java.io.tmpdir", "."), toString()).apply {
            require(mkdirs())
        }
        root = File(storage, "Playlists")
        resolver = mock {
            on { openOutputStream(any()) } doAnswer {
                FileOutputStream(it.getArgument<Uri>(0).toFile())
            }
        }
        val subdir = object : PersistentStorage.Subdirectory {
            override suspend fun tryGet(createIfAbsent: Boolean) =
                if (root.isDirectory || (createIfAbsent && root.mkdirs()))
                    DocumentFile.fromFile(root)
                else
                    null
        }
        underTest = XspfStorage(resolver, subdir)
        assertEquals(true, storage.exists())
        assertEquals(false, root.exists())
    }

    @After
    fun tearDown() {
        root.deleteRecursively()
    }

    @Test
    fun `empty root not created`() = runTest {
        assertEquals(0, underTest.enumeratePlaylists().size)
    }

    @Test
    fun `delayed root creating every time`() = runTest {
        val cursor = MatrixCursor(arrayOf())
        underTest.createPlaylist("1", cursor)
        assertEquals(true, root.exists())
        assertEquals(true, File(root, "1.xspf").exists())
        assertEquals(arrayListOf("1"), underTest.enumeratePlaylists())
        assertEquals("${root.toUri()}/1.xspf", underTest.findPlaylistUri("1").toString())
        root.deleteRecursively()

        assertEquals(false, root.exists())
        assertEquals(null, underTest.findPlaylistUri("2"))
        underTest.createPlaylist("2", cursor)
        assertEquals(true, root.exists())
        assertEquals(true, File(root, "2.xspf").exists())
        assertEquals(arrayListOf("2"), underTest.enumeratePlaylists())
        assertEquals("${root.toUri()}/2.xspf", underTest.findPlaylistUri("2").toString())
        underTest.createPlaylist("3", cursor)
        assertEquals(true, root.exists())
        assertEquals(true, File(root, "3.xspf").exists())
        assertEquals(arrayListOf("2", "3"), underTest.enumeratePlaylists())
        assertEquals("${root.toUri()}/3.xspf", underTest.findPlaylistUri("3").toString())
    }

    @Test
    fun `unsupported files ignored`() {
        root.mkdirs()
        makeFile("unsupported")
        makeFile("supported.xspf")
        makeFile("case sensitive.XSPF")
    }

    private fun makeFile(name: String) = File(root, name).apply {
        createNewFile()
    }.also {
        assertEquals(true, it.isFile && it.exists())
    }
}
