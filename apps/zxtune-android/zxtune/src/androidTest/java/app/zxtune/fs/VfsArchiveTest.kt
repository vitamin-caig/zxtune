package app.zxtune.fs

import android.content.res.Resources
import android.net.Uri
import androidx.annotation.RawRes
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import app.zxtune.io.Io.copy
import app.zxtune.io.TransactionalOutputStream
import app.zxtune.test.R
import app.zxtune.utils.StubProgressCallback
import org.junit.After
import org.junit.Assert.*
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import java.io.BufferedInputStream
import java.io.File

@RunWith(AndroidJUnit4::class)
class VfsArchiveTest {

    private lateinit var resources: Resources
    private lateinit var tmpDir: File

    private fun getFile(@RawRes res: Int, filename: String) = with(File(tmpDir, filename)) {
        val input = BufferedInputStream(resources.openRawResource(res))
        TransactionalOutputStream(this).use {
            val size = copy(input, it)
            assertTrue(size > 0)
            it.flush()
        }
        Uri.fromFile(this)
    }

    @Before
    fun setUp() {
        resources = InstrumentationRegistry.getInstrumentation().context.resources
        tmpDir = File(
            System.getProperty("java.io.tmpdir", "."),
            "VfsArchiveTest/${System.currentTimeMillis()}"
        )
    }

    @After
    fun tearDown() {
        tmpDir.listFiles()?.let { files -> files.map { File::delete } }
        tmpDir.delete()
    }

    @Test
    fun testNull() {
        try {
            VfsArchive.resolve(null)
            unreachable()
        } catch (e: NullPointerException) {
            assertNotNull("Thrown exception", e)
        }
        try {
            VfsArchive.browse(null)
            unreachable()
        } catch (e: NullPointerException) {
            assertNotNull("Thrown exception", e)
        }
        try {
            VfsArchive.browseCached(null)
            unreachable()
        } catch (e: NullPointerException) {
            assertNotNull("Thrown exception", e)
        }
    }

    @Test
    fun testDir() = Uri.parse("file:/proc").let { id ->
        (VfsArchive.resolve(id) as VfsDir).run {
            assertFalse(VfsArchive.checkIfArchive(this))
            assertEquals("proc", name)
            assertEquals("", description)
            assertEquals(id, uri)
        }
    }

    @Test
    fun testFile() = Uri.parse("file:/proc/meminfo").let { id ->
        (VfsArchive.resolve(id) as VfsFile).run {
            assertEquals("meminfo", name)
            assertEquals("", description)
            assertEquals("0", size)
            assertEquals(id, uri)
        }
    }

    @Test
    fun testFileTrack() = getFile(R.raw.track, "track").let { id ->
        val file = (VfsArchive.resolve(id) as VfsFile).apply {
            assertEquals("track", name)
            assertEquals("", description)
            assertEquals("2.8K", size)
        }
        VfsArchive.browse(file).run {
            assertEquals(file, this)
        }
        VfsArchive.browseCached(file).run {
            assertEquals(file, this)
        }
        VfsArchive.resolve(id)!!.run {
            assertEquals(file.uri, uri)
        }
    }

    @Test
    fun testGzippedTrack() = getFile(R.raw.gzipped, "gzipped").let { id ->
        val file = (VfsArchive.resolve(id) as VfsFile).apply {
            assertEquals("gzipped", name)
            assertEquals("", description)
            assertEquals("54.1K", size)
        }
        val normalizedUri = file.uri
        val subUri = normalizedUri.buildUpon().fragment("+unGZIP").build()
        try {
            VfsArchive.resolve(subUri)
            unreachable()
        } catch (e: Exception) {
            assertNotNull("Thrown exception", e)
            assertEquals("No archive found", e.message)
        }
        VfsArchive.browse(file).run {
            assertEquals(file, this)
        }
        VfsArchive.browseCached(file).run {
            assertEquals(file, this)
        }
        VfsArchive.resolve(id)!!.run {
            assertEquals(normalizedUri, uri)
        }
        (VfsArchive.resolve(subUri) as VfsFile).run {
            assertEquals(subUri, uri)
            assertEquals("+unGZIP", name)
            assertEquals("sll3", description)
            assertEquals("2:33", size)

            // no parent dir really, so just file itself
            (parent as VfsFile).run {
                assertEquals(normalizedUri, uri)
            }
        }
    }

    @Test
    fun testMultitrack() = getFile(R.raw.multitrack, "multitrack").let { id ->
        val file = (VfsArchive.resolve(id) as VfsFile).apply {
            assertEquals("multitrack", name)
            assertEquals("", description)
            assertEquals("10.3K", size)
        }
        val normalizedUri = file.uri
        val subUri = normalizedUri.buildUpon().fragment("#2").build()
        try {
            VfsArchive.resolve(subUri)
            unreachable()
        } catch (e: Exception) {
            assertNotNull("Thrown exception", e)
            assertEquals("No archive found", e.message)
        }
        (VfsArchive.browse(file) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(normalizedUri, uri)
        }
        (VfsArchive.browseCached(file) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(normalizedUri, uri)
        }
        (VfsArchive.resolve(id) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(normalizedUri, uri)
            assertEquals(
                """
            {#1} (RoboCop 3 - Jeroen Tel) <3:56>
            {#2} (RoboCop 3 - Jeroen Tel) <0:09>
            {#3} (RoboCop 3 - Jeroen Tel) <0:16>
            {#4} (RoboCop 3 - Jeroen Tel) <0:24>
            {#5} (RoboCop 3 - Jeroen Tel) <0:24>
            {#6} (RoboCop 3 - Jeroen Tel) <0:24>
            {#7} (RoboCop 3 - Jeroen Tel) <0:12>
            {#8} (RoboCop 3 - Jeroen Tel) <0:35>
            {#9} (RoboCop 3 - Jeroen Tel) <0:01>
            {#10} (RoboCop 3 - Jeroen Tel) <0:01>
            {#11} (RoboCop 3 - Jeroen Tel) <0:01>
            {#12} (RoboCop 3 - Jeroen Tel) <0:02>
            {#13} (RoboCop 3 - Jeroen Tel) <0:01>
            {#14} (RoboCop 3 - Jeroen Tel) <0:01>
            {#15} (RoboCop 3 - Jeroen Tel) <0:01>
            {#16} (RoboCop 3 - Jeroen Tel) <0:05>
            {#17} (RoboCop 3 - Jeroen Tel) <0:01>
            {#18} (RoboCop 3 - Jeroen Tel) <0:01>
            {#19} (RoboCop 3 - Jeroen Tel) <0:03>
            {#20} (RoboCop 3 - Jeroen Tel) <0:02>
            
            """.trimIndent(), listContent(this)
            )
        }
        (VfsArchive.resolve(subUri) as VfsFile).run {
            assertEquals(subUri, uri)
            assertEquals("#2", name)
            assertEquals("RoboCop 3 - Jeroen Tel", description)
            assertEquals("0:09", size)

            // archive dir overrides file
            (parent as VfsDir).run {
                assertEquals(normalizedUri, uri)
                assertTrue(VfsArchive.checkIfArchive(this))
                (parent as VfsDir).run {
                    assertFalse(VfsArchive.checkIfArchive(this))
                }
            }
        }
    }

    @Test
    fun testArchive() = getFile(R.raw.archive, "archive").let { id ->
        val file = (VfsArchive.resolve(id) as VfsFile).apply {
            assertEquals("archive", name)
            assertEquals("", description)
            assertEquals("2.4K", size)
        }
        val normalizedUri = file.uri
        val subFileUri = normalizedUri.buildUpon().fragment("auricom.pt3").build()
        try {
            VfsArchive.resolve(subFileUri)
            unreachable()
        } catch (e: Exception) {
            assertNotNull("Thrown exception", e)
            assertEquals("No archive found", e.message)
        }
        val subDirUri = normalizedUri.buildUpon().fragment("coop-Jeffie").build()
        try {
            VfsArchive.resolve(subDirUri)
            unreachable()
        } catch (e: Exception) {
            assertNotNull("Thrown exception", e)
            assertEquals("No archive found", e.message)
        }
        (VfsArchive.browse(file) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(normalizedUri, uri)
        }
        (VfsArchive.browseCached(file) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(normalizedUri, uri)
        }
        (VfsArchive.resolve(id) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(normalizedUri, uri)
            assertEquals(
                """{auricom.pt3} (auricom - sclsmnn^mc ft frolic&fo(?). 2015) <1:13>
[coop-Jeffie] ()
 {bass sorrow.pt3} (bass sorrow - scalesmann^mc & jeffie/bw, 2004) <2:10>
""",
                listContent(this)
            )
        }
        (VfsArchive.resolve(subDirUri) as VfsDir).run {
            assertEquals(subDirUri, uri)
            assertEquals("coop-Jeffie", name)
            assertEquals("", description)
        }
        (VfsArchive.resolve(subFileUri) as VfsFile).run {
            assertEquals(subFileUri, uri)
            assertEquals("auricom.pt3", name)
            assertEquals("auricom - sclsmnn^mc ft frolic&fo(?). 2015", description)
            assertEquals("1:13", size)

            // archive dir overrides file
            (parent as VfsDir).run {
                assertEquals(normalizedUri, uri)
                assertTrue(VfsArchive.checkIfArchive(this))
                (parent as VfsDir).run {
                    assertFalse(VfsArchive.checkIfArchive(this))
                }
            }
        }
    }

    @Test
    fun testForceResolve() {
        getFile(R.raw.gzipped, "gzipped").let { id ->
            val file = VfsArchive.resolve(id) as VfsFile
            val normalizedUri = file.uri
            val subUri = normalizedUri.buildUpon().fragment("+unGZIP").build()
            (VfsArchive.resolveForced(subUri, StubProgressCallback.instance()) as VfsFile).run {
                assertEquals(subUri, uri)
                assertEquals("+unGZIP", name)
                assertEquals("sll3", description)
                assertEquals("2:33", size)

                // no parent dir really, so just file itself
                (parent as VfsFile).run {
                    assertEquals(normalizedUri, uri)
                }
            }
        }
        getFile(R.raw.multitrack, "multitrack").let { id ->
            val file = VfsArchive.resolve(id) as VfsFile
            val normalizedUri = file.uri
            val subUri = normalizedUri.buildUpon().fragment("#2").build()
            (VfsArchive.resolveForced(subUri, StubProgressCallback.instance()) as VfsFile).run {
                assertEquals(subUri, uri)
                assertEquals("#2", name)
                assertEquals("RoboCop 3 - Jeroen Tel", description)
                assertEquals("0:09", size)

                // archive dir overrides file
                (parent as VfsDir).run {
                    assertEquals(normalizedUri, uri)
                    assertTrue(VfsArchive.checkIfArchive(this))
                    (parent as VfsDir).run {
                        assertFalse(VfsArchive.checkIfArchive(this))
                    }
                }
            }
        }

        getFile(R.raw.archive, "archive").let { id ->
            val file = VfsArchive.resolve(id) as VfsFile
            val normalizedUri = file.uri
            val subFileUri = normalizedUri.buildUpon().fragment("auricom.pt3").build()

            (VfsArchive.resolveForced(subFileUri, StubProgressCallback.instance()) as VfsFile).run {
                assertEquals(subFileUri, uri)
                assertEquals("auricom.pt3", name)
                assertEquals("auricom - sclsmnn^mc ft frolic&fo(?). 2015", description)
                assertEquals("1:13", size)

                // archive dir overrides file
                (parent as VfsDir).run {
                    assertEquals(normalizedUri, uri)
                    assertTrue(VfsArchive.checkIfArchive(this))
                    (parent as VfsDir).run {
                        assertFalse(VfsArchive.checkIfArchive(this))
                    }
                }
            }
        }
        getFile(R.raw.archive, "archive2").let { id ->
            val file = VfsArchive.resolve(id) as VfsFile
            val normalizedUri = file.uri
            val subDirUri = normalizedUri.buildUpon().fragment("coop-Jeffie").build()
            (VfsArchive.resolveForced(subDirUri, StubProgressCallback.instance()) as VfsDir).run {
                assertEquals(subDirUri, uri)
                assertEquals("coop-Jeffie", name)
                assertEquals("", description)
            }
        }
    }
}

private fun unreachable() {
    fail("Unreachable")
}

private fun listContent(dir: VfsDir) = with(StringBuilder()) {
    listDir(dir, this, "")
    toString()
}

private fun listDir(input: VfsDir, list: StringBuilder, tab: String) {
    try {
        input.enumerate(object : VfsDir.Visitor() {
            override fun onDir(dir: VfsDir) {
                list.append(
                    String.format(
                        "%s[%s] (%s)\n",
                        tab,
                        dir.name,
                        dir.description
                    )
                )
                listDir(dir, list, "$tab ")
            }

            override fun onFile(file: VfsFile) {
                list.append(
                    String.format(
                        "%s{%s} (%s) <%s>\n",
                        tab,
                        file.name,
                        file.description,
                        file.size
                    )
                )
            }
        })
    } catch (e: Exception) {
        list.append(e)
    }
}
