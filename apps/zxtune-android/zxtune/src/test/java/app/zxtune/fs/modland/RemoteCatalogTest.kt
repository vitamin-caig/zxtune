package app.zxtune.fs.modland

import androidx.collection.SparseArrayCompat
import app.zxtune.BuildConfig
import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import org.junit.Assert.assertEquals
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class RemoteCatalogTest {

    private lateinit var catalog: RemoteCatalog

    @Before
    fun setUp() = with(HttpProviderFactory.createTestProvider()) {
        catalog = RemoteCatalog(MultisourceHttpProvider(this))
    }

    @Test
    fun `test authors`() {
        val checkpoints = SparseArrayCompat<Group>().apply {
            // first
            append(0, Group(25189, "$4753 Softcopy", 33))
            // last
            append(91, Group(13177, "9Nobo8", 1))
        }

        with(GroupsChecker()) {
            catalog.getAuthors().queryGroups("#", this, this)
            checkGroups(92, checkpoints)
            checkProgress(92, 92)
        }
    }

    @Test
    fun `test authors tracks`() {
        val checkpoints = SparseArrayCompat<Track>().apply {
            // first
            append(
                0,
                Track(
                    "/pub/modules/Nintendo%20SPC/4-Mat/Micro%20Machines/01%20-%20main%20theme.spc",
                    66108
                )
            )
            // last (100th really)
            append(99, Track("/pub/modules/Fasttracker%202/4-Mat/blade1.xm", 24504))
        }

        with(TracksChecker()) {
            catalog.getAuthors().queryTracks(374 /*4-Mat*/, this, this)
            checkTracks(100, checkpoints)
            // two full pages processed
            checkProgress(80, 744)
        }
    }

    @Test
    fun `test collections`() {
        val checkpoints = SparseArrayCompat<Group>().apply {
            // first
            append(0, Group(6282, "O&F", 10))
            // last
            append(115, Group(6422, "Ozzyoss", 5))
        }

        with(GroupsChecker()) {
            catalog.getCollections().queryGroups("O", this, this)
            checkGroups(116, checkpoints)
            checkProgress(116, 116)
        }
    }

    @Test
    fun `test formats`() {
        val checkpoints = SparseArrayCompat<Group>().apply {
            // first
            append(0, Group(212, "Mad Tracker 2", 93))
            // last
            append(26, Group(268, "MVX Module", 12))
        }

        with(GroupsChecker()) {
            catalog.getFormats().queryGroups("M", this, this)
            // single page
            checkGroups(27, checkpoints)
            checkProgress(27, 27)
        }
    }

    @Test
    fun `test getTrackUris`() = with(RemoteCatalog.getTrackUris("/pub/modules/track.gz")) {
        assertEquals(2L, size.toLong())
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/modland/pub/modules/track.gz",
            get(0).toString()
        )
        assertEquals(
            "http://ftp.amigascne.org/mirrors/ftp.modland.com/pub/modules/track.gz",
            get(1).toString()
        )
    }

    private class GroupsChecker : Catalog.GroupsVisitor,
        ProgressCallback {

        val result = ArrayList<Group>()
        val lastProgress = intArrayOf(-1, -1)

        override fun onProgressUpdate(done: Int, total: Int) {
            lastProgress[0] = done
            lastProgress[1] = total
        }

        override fun accept(obj: Group) {
            result.add(obj)
        }

        fun checkGroups(minSize: Int, checkpoints: SparseArrayCompat<Group>) {
            assertTrue(result.size >= minSize)
            for (i in 0 until checkpoints.size()) {
                val pos = checkpoints.keyAt(i)
                val ref = checkpoints.valueAt(i)
                val real = result[pos]
                assertEquals(ref.id, real.id)
                assertEquals(ref.name, real.name)
                assertEquals(ref.tracks, real.tracks)
            }
        }

        fun checkProgress(minDone: Int, minTotal: Int) {
            assertTrue(lastProgress[0] >= minDone)
            assertTrue(lastProgress[1] >= minTotal)
        }
    }

    private class TracksChecker : Catalog.TracksVisitor,
        ProgressCallback {

        val result = ArrayList<Track>()
        val lastProgress = intArrayOf(-1, -1)

        override fun onProgressUpdate(done: Int, total: Int) {
            lastProgress[0] = done
            lastProgress[1] = total
        }

        override fun accept(obj: Track): Boolean {
            result.add(obj)
            return result.size < 100
        }

        fun checkTracks(minSize: Int, checkpoints: SparseArrayCompat<Track>) {
            assertTrue(result.size >= minSize)
            for (i in 0 until checkpoints.size()) {
                val pos = checkpoints.keyAt(i)
                val ref = checkpoints.valueAt(i)
                val real = result[pos]
                assertEquals(ref.filename, real.filename)
                assertEquals(ref.path, real.path)
                assertEquals(ref.size, real.size)
                assertEquals(ref.id, real.id)
            }
        }

        fun checkProgress(done: Int, total: Int) {
            assertEquals(done, lastProgress[0])
            assertEquals(total, lastProgress[1])
        }
    }
}
