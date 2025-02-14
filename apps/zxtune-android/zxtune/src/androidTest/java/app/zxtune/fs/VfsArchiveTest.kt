package app.zxtune.fs

import android.content.res.Resources
import android.net.Uri
import androidx.annotation.RawRes
import androidx.test.ext.junit.runners.AndroidJUnit4
import androidx.test.platform.app.InstrumentationRegistry
import app.zxtune.assertThrows
import app.zxtune.io.Io.copy
import app.zxtune.io.TransactionalOutputStream
import app.zxtune.test.R
import app.zxtune.utils.StubProgressCallback
import org.junit.After
import org.junit.Assert.assertArrayEquals
import org.junit.Assert.assertEquals
import org.junit.Assert.assertFalse
import org.junit.Assert.assertSame
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import java.io.BufferedInputStream
import java.io.File
import java.io.IOException

@RunWith(AndroidJUnit4::class)
class VfsArchiveTest {

    private lateinit var resources: Resources
    private lateinit var tmpDir: File

    private fun getFile(@RawRes res: Int, filename: String) = with(File(tmpDir, filename)) {
        BufferedInputStream(resources.openRawResource(res)).use { input ->
            TransactionalOutputStream(this).use {
                val size = copy(input, it)
                assertTrue(size > 0)
                it.flush()
            }
        }
        Uri.Builder().scheme("file").path(path).build()
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
        tmpDir.listFiles()?.map(File::delete)
        tmpDir.delete()
    }

    @Test
    fun testNull() {
        assertThrows<NullPointerException> {
            VfsArchive.resolve(null)
        }
        assertThrows<NullPointerException> {
            VfsArchive.browse(null)
        }
        assertThrows<NullPointerException> {
            VfsArchive.browseCached(null)
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
    fun testFile() {
        assertResolvedAsFile(Uri.parse("file:/proc/meminfo"), expectedSize = "0")
    }

    @Test
    fun testFileTrack() = getFile(R.raw.track, "track").let { id ->
        val file = assertResolvedAsFile(id, expectedSize = "2.8K")
        assertSame(file, VfsArchive.browse(file))
        assertSame(file, VfsArchive.browseCached(file))
        assertEquals(file.uri, VfsArchive.resolve(id)?.uri)
    }

    @Test
    fun testGzippedTrack() = getFile(R.raw.gzipped, "gzipped").let { id ->
        val file = assertResolvedAsFile(id, expectedSize = "54.1K")
        val subUri = file.uri.withFragment("+unGZIP")
        assertThrows<IOException> {
            VfsArchive.resolve(subUri)
        }.let { e ->
            assertEquals("No archive found", e.message)
        }
        assertEquals(file, VfsArchive.browse(file))
        assertEquals(file, VfsArchive.browseCached(file))
        assertEquals(file.uri, VfsArchive.resolve(id)?.uri)
        assertResolvedAsFile(
            subUri, expectedName = "gzipped", expectedDescription = "sll3", expectedSize = "2:33"
        ).run {
            // no parent dir really, so just file itself
            assertEquals(file.uri, (parent as VfsFile).uri)
        }
    }

    @Test
    fun testMultitrack() = getFile(R.raw.multitrack, "multitrack").let { id ->
        val file = assertResolvedAsFile(id, expectedSize = "16.2K")
        val subUri = file.uri.withFragment("#2")
        assertThrows<IOException> {
            VfsArchive.resolve(subUri)
        }.let { e ->
            assertEquals("No archive found", e.message)
        }
        (VfsArchive.browse(file) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(file.uri, uri)
        }
        (VfsArchive.browseCached(file) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(file.uri, uri)
        }
        (VfsArchive.resolve(id) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(file.uri, uri)
            assertArrayEquals(
                arrayOf(
                    File(0, "#1", "#1", "Title - Tim and/or Geoff Follin", "0:16"),
                    File(0, "#2", "#2", "Looney Tunes - Tim and/or Geoff Follin", "3:12"),
                    File(0, "#3", "#3", "Like Cats And Mice - Tim and/or Geoff Follin", "3:16"),
                    File(0, "#4", "#4", "Victory is Mine! - Tim and/or Geoff Follin", "0:10"),
                    File(0, "#5", "#5", "Game Over - Tim and/or Geoff Follin", "0:10"),
                ), listContent(this)
            )
        }
        assertResolvedAsFile(
            subUri,
            expectedName = "#2",
            expectedDescription = "Looney Tunes - Tim and/or Geoff Follin",
            expectedSize = "3:12"
        ).run {
            // archive dir overrides file
            (parent as VfsDir).run {
                assertEquals(file.uri, uri)
                assertTrue(VfsArchive.checkIfArchive(this))
                assertFalse(VfsArchive.checkIfArchive(parent as VfsDir))
            }
        }
    }

    @Test
    fun testArchive() = getFile(R.raw.archive, "archive").let { id ->
        val file = assertResolvedAsFile(id, expectedSize = "2.4K")
        val subFileUri = file.uri.withFragment("auricom.pt3")
        assertThrows<IOException> {
            VfsArchive.resolve(subFileUri)
        }.let { e ->
            assertEquals("No archive found", e.message)
        }
        val subDirUri = file.uri.withFragment("coop-Jeffie")
        assertThrows<IOException> {
            VfsArchive.resolve(subDirUri)
        }.let { e ->
            assertEquals("No archive found", e.message)
        }
        (VfsArchive.browse(file) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(file.uri, uri)
        }
        (VfsArchive.browseCached(file) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(file.uri, uri)
        }
        (VfsArchive.resolve(id) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(file.uri, uri)
            assertArrayEquals(
                arrayOf(
                    File(
                        0,
                        "auricom.pt3",
                        "auricom.pt3",
                        "auricom - sclsmnn^mc ft frolic&fo(?). 2015",
                        "1:13"
                    ),
                    Dir(0, "coop-Jeffie", "coop-Jeffie"),
                    File(
                        1,
                        "coop-Jeffie/bass sorrow.pt3",
                        "bass sorrow.pt3",
                        "bass sorrow - scalesmann^mc & jeffie/bw, 2004",
                        "2:10"
                    ),
                ), listContent(this)
            )
        }
        (VfsArchive.resolve(subDirUri) as VfsDir).run {
            assertEquals(subDirUri, uri)
            assertEquals("coop-Jeffie", name)
            assertEquals("", description)
            (parent as VfsDir).run {
                assertEquals(file.uri, uri)
                assertTrue(VfsArchive.checkIfArchive(this))
                assertFalse(VfsArchive.checkIfArchive(parent as VfsDir))
            }
            assertArrayEquals(
                arrayOf(
                    File(
                        0,
                        "coop-Jeffie/bass sorrow.pt3",
                        "bass sorrow.pt3",
                        "bass sorrow - scalesmann^mc & jeffie/bw, 2004",
                        "2:10"
                    ),
                ), listContent(this)
            )
        }
        assertResolvedAsFile(
            subFileUri,
            expectedName = "auricom.pt3",
            expectedDescription = "auricom - sclsmnn^mc ft frolic&fo(?). 2015",
            expectedSize = "1:13"
        ).run {
            // archive dir overrides file
            (parent as VfsDir).run {
                assertEquals(file.uri, uri)
                assertTrue(VfsArchive.checkIfArchive(this))
                assertFalse(VfsArchive.checkIfArchive(parent as VfsDir))
            }
        }
    }

    @Test
    fun testArchiveWithGzips() = getFile(R.raw.archive_with_gzips, "archive_with_gzips").let { id ->
        val file = assertResolvedAsFile(id, expectedSize = "67.2K")
        (VfsArchive.browse(file) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(file.uri, uri)
        }
        (VfsArchive.browseCached(file) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(file.uri, uri)
        }
        (VfsArchive.resolve(id) as VfsDir).run {
            assertTrue(VfsArchive.checkIfArchive(this))
            assertEquals(file.uri, uri)
            assertArrayEquals(
                arrayOf(
                    File(0, "gzipped/+unGZIP", "gzipped", "sll3", "2:33"),
                    Dir(0, "gzipped_archive/+unGZIP", "gzipped_archive"),
                    File(
                        1,
                        "gzipped_archive/+unGZIP/auricom.pt3",
                        "auricom.pt3",
                        "auricom - sclsmnn^mc ft frolic&fo(?). 2015",
                        "1:13"
                    ),
                    Dir(1, "gzipped_archive/+unGZIP/coop-Jeffie", "coop-Jeffie"),
                    File(
                        2,
                        "gzipped_archive/+unGZIP/coop-Jeffie/bass sorrow.pt3",
                        "bass sorrow.pt3",
                        "bass sorrow - scalesmann^mc & jeffie/bw, 2004",
                        "2:10"
                    ),
                    Dir(0, "gzipped_multitrack/+unGZIP", "gzipped_multitrack"),
                    File(
                        1,
                        "gzipped_multitrack/+unGZIP/#1",
                        "#1",
                        "Title - Tim and/or Geoff Follin",
                        "0:16"
                    ),
                    File(
                        1,
                        "gzipped_multitrack/+unGZIP/#2",
                        "#2",
                        "Looney Tunes - Tim and/or " + "Geoff Follin",
                        "3:12"
                    ),
                    File(
                        1,
                        "gzipped_multitrack/+unGZIP/#3",
                        "#3",
                        "Like Cats And Mice - Tim " + "and/or Geoff Follin",
                        "3:16"
                    ),
                    File(
                        1,
                        "gzipped_multitrack/+unGZIP/#4",
                        "#4",
                        "Victory is Mine! - Tim" + " and/or Geoff Follin",
                        "0:10"
                    ),
                    File(
                        1,
                        "gzipped_multitrack/+unGZIP/#5",
                        "#5",
                        "Game Over - Tim " + "and/or Geoff Follin",
                        "0:10"
                    ),
                    File(
                        0,
                        "track",
                        "track",
                        "AsSuRed ... Hi! My Frends ... - Mm<M of Sage 14.Apr.XX twr 00:37",
                        "3:41"
                    ),
                ), listContent(this)
            )
            assertFalse(VfsArchive.checkIfArchive(parent as VfsDir))
        }
        val gzippedArchiveId = id.withFragment("gzipped_archive")
        assertThrows<IOException> {
            VfsArchive.resolve(gzippedArchiveId)
        }
        val gzippedArchiveSubdirId = id.withFragment("gzipped_archive/+unGZIP")
        (VfsArchive.resolve(gzippedArchiveSubdirId) as VfsDir).run {
            assertEquals("gzipped_archive", name)
            assertEquals(gzippedArchiveSubdirId, uri)
            (parent as VfsDir).run {
                assertEquals(file.uri, uri)
            }
        }
        val gzippedMultitrackId = id.withFragment("gzipped_multitrack")
        assertThrows<IOException> {
            VfsArchive.resolve(gzippedMultitrackId)
        }
        val gzippedMultitrackSubdirId = id.withFragment("gzipped_multitrack/+unGZIP")
        (VfsArchive.resolve(gzippedMultitrackSubdirId) as VfsDir).run {
            assertEquals("gzipped_multitrack", name)
            assertEquals(gzippedMultitrackSubdirId, uri)
            (parent as VfsDir).run {
                assertEquals(file.uri, uri)
            }
        }
    }

    @Test
    fun testForceResolve() {
        getFile(R.raw.gzipped, "gzipped").let { id ->
            val file = assertResolvedAsFile(id, expectedSize = "54.1K")
            val subUri = file.uri.withFragment("+unGZIP")
            (VfsArchive.resolveForced(subUri, StubProgressCallback.instance()) as VfsFile).run {
                assertEquals(subUri, uri)
                assertEquals("gzipped", name)
                assertEquals("sll3", description)
                assertEquals("2:33", size)

                // no parent dir really, so just file itself
                assertEquals(file.uri, (parent as VfsFile).uri)
            }
        }
        getFile(R.raw.multitrack, "multitrack").let { id ->
            val file = assertResolvedAsFile(id, expectedSize = "16.2K")
            val subUri = file.uri.withFragment("#2")
            (VfsArchive.resolveForced(subUri, StubProgressCallback.instance()) as VfsFile).run {
                assertEquals(subUri, uri)
                assertEquals("#2", name)
                assertEquals("Looney Tunes - Tim and/or Geoff Follin", description)
                assertEquals("3:12", size)

                // archive dir overrides file
                (parent as VfsDir).run {
                    assertEquals(file.uri, uri)
                    assertTrue(VfsArchive.checkIfArchive(this))
                    (parent as VfsDir).run {
                        assertFalse(VfsArchive.checkIfArchive(this))
                    }
                }
            }
        }

        getFile(R.raw.archive, "archive").let { id ->
            val file = assertResolvedAsFile(id, expectedSize = "2.4K")
            val subFileUri = file.uri.withFragment("auricom.pt3")

            (VfsArchive.resolveForced(
                subFileUri, StubProgressCallback.instance()
            ) as VfsFile).run {
                assertEquals(subFileUri, uri)
                assertEquals("auricom.pt3", name)
                assertEquals("auricom - sclsmnn^mc ft frolic&fo(?). 2015", description)
                assertEquals("1:13", size)

                // archive dir overrides file
                (parent as VfsDir).run {
                    assertEquals(file.uri, uri)
                    assertTrue(VfsArchive.checkIfArchive(this))
                    (parent as VfsDir).run {
                        assertFalse(VfsArchive.checkIfArchive(this))
                    }
                }
            }
        }
        getFile(R.raw.archive, "archive2").let { id ->
            val file = assertResolvedAsFile(id, expectedSize = "2.4K")
            val subDirUri = file.uri.withFragment("coop-Jeffie")
            (VfsArchive.resolveForced(
                subDirUri, StubProgressCallback.instance()
            ) as VfsDir).run {
                assertEquals(subDirUri, uri)
                assertEquals("coop-Jeffie", name)
                assertEquals("", description)
            }
        }
    }
}

private fun assertResolvedAsFile(
    id: Uri,
    expectedName: String = requireNotNull(id.lastPathSegment),
    expectedDescription: String = "",
    expectedSize: String
) = (VfsArchive.resolve(id) as VfsFile).apply {
    assertEquals(id, uri)
    assertEquals(expectedName, name)
    assertEquals(expectedDescription, description)
    assertEquals(expectedSize, size)
}

fun Uri.withFragment(fragment: String): Uri = buildUpon().fragment(fragment).build()

sealed class Entry

data class File(
    val depth: Int, val subpath: String, val name: String, val description: String, val size: String
) : Entry()

data class Dir(val depth: Int, val subpath: String, val name: String) : Entry()

private fun listContent(dir: VfsDir) = ArrayList<Entry>().apply {
    listDir(dir, this, 0)
}.toArray()

private fun listDir(input: VfsDir, list: ArrayList<Entry>, depth: Int) {
    input.enumerate(object : VfsDir.Visitor() {
        override fun onDir(dir: VfsDir) = with(dir) {
            require(description.isEmpty())
            list.add(Dir(depth, requireNotNull(uri.fragment), name))
            listDir(dir, list, depth + 1)
        }

        override fun onFile(file: VfsFile) = with(file) {
            list.add(File(depth, requireNotNull(uri.fragment), name, description, size))
            Unit
        }
    })
}
