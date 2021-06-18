package app.zxtune.playback

import android.content.Context
import android.content.res.Resources
import android.net.Uri
import androidx.annotation.RawRes
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import app.zxtune.fs.Vfs
import app.zxtune.io.Io.copy
import app.zxtune.io.TransactionalOutputStream
import app.zxtune.test.R
import org.junit.After
import org.junit.Assert
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import java.io.BufferedInputStream
import java.io.File
import java.util.*

@RunWith(AndroidJUnit4::class)
class FileIteratorTest {
    private lateinit var resources: Resources
    private lateinit var tmpDir: File
    private lateinit var context: Context

    private fun getFile(@RawRes res: Int, filename: String) = with(File(tmpDir, filename)) {
        val input = BufferedInputStream(resources.openRawResource(res))
        TransactionalOutputStream(this).use { out ->
            val size = copy(input, out)
            Assert.assertTrue(size > 0)
            out.flush()
        }
        Vfs.resolve(Uri.fromFile(this)).uri
    }

    @Before
    fun setUp() {
        context = InstrumentationRegistry.getInstrumentation().context
        resources = context.resources
        tmpDir = File(
            System.getProperty("java.io.tmpdir", "."),
            "FileIteratorTest/${System.currentTimeMillis()}"
        )
    }

    @After
    fun tearDown() {
        tmpDir.listFiles()?.let { files -> files.map { File::delete } }
        tmpDir.delete()
    }

    @Test
    fun testTrack() {
        getFile(R.raw.multitrack, "0")
        val track = getFile(R.raw.track, "1")
        getFile(R.raw.gzipped, "2")
        val items = ArrayList<PlayableItem>().apply {
            val iter = FileIterator.create(context, track)
            do {
                add(iter.item)
            } while (iter.next())
        }
        assertEquals(2, items.size.toLong())
        assertEquals("AsSuRed ... Hi! My Frends ...", items[0].title)
        assertEquals("sll3", items[1].title)
    }

    @Test
    fun testSingleArchived() {
        getFile(R.raw.multitrack, "0")
        val gzipped = getFile(R.raw.gzipped, "1")
        getFile(R.raw.track, "2")
        val items = ArrayList<PlayableItem>().apply {
            val iter = FileIterator.create(context, gzipped.withFragment("+unGZIP"))
            do {
                add(iter.item)
            } while (iter.next())
        }
        assertEquals(2, items.size.toLong())
        assertEquals("sll3", items[0].title)
        assertEquals("AsSuRed ... Hi! My Frends ...", items[1].title)
    }

    @Test
    fun testArchivedElement() {
        getFile(R.raw.multitrack, "0")
        val archived = getFile(R.raw.archive, "1")
        getFile(R.raw.track, "2")
        val items = ArrayList<PlayableItem>().apply {
            val iter =
                FileIterator.create(context, archived.withFragment("coop-Jeffie/bass sorrow.pt3"))
            do {
                add(iter.item)
            } while (iter.next())
        }
        assertEquals(1, items.size.toLong())
        assertEquals("bass sorrow", items[0].title)
    }

    @Test
    fun testMultitrackElement() {
        getFile(R.raw.gzipped, "0")
        val multitrack = getFile(R.raw.multitrack, "1")
        getFile(R.raw.track, "2")
        val items = ArrayList<PlayableItem>().apply {
            val iter = FileIterator.create(context, multitrack.withFragment("#2"))
            do {
                add(iter.item)
            } while (iter.next())
        }
        assertEquals(19, items.size.toLong())
        assertEquals("RoboCop 3", items[0].title)
    }
}

private fun Uri.withFragment(fragment: String) = buildUpon().fragment(fragment).build()
