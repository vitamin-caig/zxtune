package app.zxtune.fs.modarchive

import androidx.test.ext.junit.runners.AndroidJUnit4
import app.zxtune.BuildConfig
import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import org.junit.Assert
import org.junit.Assert.assertTrue
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.*

@RunWith(AndroidJUnit4::class)
class RemoteCatalogTest {

    private lateinit var catalog: RemoteCatalog

    @Before
    fun setUp() = with(HttpProviderFactory.createTestProvider()) {
        catalog = RemoteCatalog(MultisourceHttpProvider(this), BuildConfig.MODARCHIVE_KEY)
    }

    @Test
    fun test_queryAuthors() {
        val checkpoints = arrayOf(
            Author(68877, "_Matt_"),
            Author(83394, "4mat"),
            Author(0, "!Unknown"),
            Author(91065, "ㅤㅤㅤㅤㅤㅤㅤㅤㅤㅤㅤ"),
            Author(90081, "Дима")
        )
        val result = ArrayList<Author>()
        val progress = mock<ProgressCallback>()

        catalog.queryAuthors(object : Catalog.AuthorsVisitor {
            override fun setCountHint(count: Int) {
                result.ensureCapacity(count)
            }

            override fun accept(obj: Author) {
                result.add(obj)
            }
        }, progress)

        check(2400, checkpoints, result)
        verifyProgress(progress, 61)
    }

    @Test
    fun test_queryGenres() {
        val checkpoints = arrayOf(
            Genre(48, "Alternative", 440),
            Genre(71, "Trance (general)", 910),
            Genre(25, "Soul", 10)
        )
        val result = ArrayList<Genre>()

        catalog.queryGenres(object : Catalog.GenresVisitor {
            override fun setCountHint(count: Int) {
                result.ensureCapacity(count)
            }

            override fun accept(obj: Genre) {
                result.add(obj)
            }
        })

        check(75, checkpoints, result)
    }

    @Test
    fun test_queryTracks_by_author() {
        val checkpoints = arrayOf(
            Track(182782, "4mat_-_broken_heart.xm", "<3 broken heart <3", 61760),
            Track(77970, "forest-loader.mod", "forest-loader", 24992),
            Track(140500, "egg_plant.xm", "<< EGG PLANT >>", 530975)
        )
        val result = ArrayList<Track>()
        val progress = mock<ProgressCallback>()

        catalog.queryTracks(Author(83394, "UNUSED!!!"), object : Catalog.TracksVisitor {
            override fun setCountHint(count: Int) {
                result.ensureCapacity(count)
            }

            override fun accept(obj: Track) {
                result.add(obj)
            }
        }, progress)

        check(370, checkpoints, result)
        verifyProgress(progress, 10)
    }

    @Test
    fun test_queryTracks_by_genre() {
        val checkpoints = arrayOf(
            Track(94399, "_nice_outfit_.mod", "(.nice outfit.)", 43386),
            Track(190055, "joseph_tek_fox_-_ci_towermpa.s3m", "Tower of Masamune Past", 235184),
            Track(162204, "zulualert.mod", "zulu alert 3", 196092)
        )
        val result = ArrayList<Track>()
        val progress = mock<ProgressCallback>()

        catalog.queryTracks(Genre(48, "UNUSED!!!", -100500), object : Catalog.TracksVisitor {
            override fun setCountHint(count: Int) {
                result.ensureCapacity(count)
            }

            override fun accept(obj: Track) {
                result.add(obj)
            }
        }, progress)

        check(440, checkpoints, result)
        verifyProgress(progress, 10)
    }

    @Test
    fun test_findTracks() {
        val authorsCheckpoints = arrayOf(
            Author(69008, "xtd"),
            Author(0, "!Unknown"),
        )
        val tracksCheckpoints = arrayOf(
            Track(121407, "_chipek_test_.mod", "## chipek test ##", 13740),
            Track(49744, "ncontest.s3m", "no contest", 124440),
            Track(111269, "wildtest.mod", "wildtest", 2932)
        )
        val authorsResult = ArrayList<Author>()
        val tracksResult = ArrayList<Track>()

        catalog.findTracks("test", object : Catalog.FoundTracksVisitor {
            override fun setCountHint(count: Int) {
                authorsResult.ensureCapacity(count)
                tracksResult.ensureCapacity(count)
            }

            override fun accept(author: Author, track: Track) {
                authorsResult.add(author)
                tracksResult.add(track)
            }
        })

        check(260, authorsCheckpoints, authorsResult)
        check(260, tracksCheckpoints, tracksResult)
    }

    @Test
    fun test_findRandomTracks() {
        val visitor = mock<Catalog.TracksVisitor>()

        catalog.findRandomTracks(visitor)

        verify(visitor, never()).setCountHint(any<Int>())
        verify(visitor, atLeastOnce()).accept(any<Track>())
    }
}

private fun <T> check(minSize: Int, expected: Array<T>, actual: ArrayList<T>) {
    assertTrue(actual.size >= minSize)
    for (obj in expected) {
        assertTrue("Not found $obj", contains(actual, obj))
    }
}

private fun <T> contains(actual: ArrayList<T>, obj: T) = actual.find { matches(it, obj) } != null

private fun <T> matches(lh: T, rh: T) = when (lh) {
    is Author -> lh == rh as Author
    is Genre -> with(rh as Genre) { lh.id == id && lh.name == name && lh.tracks >= tracks }
    is Track -> lh == rh as Track
    else -> {
        Assert.fail("Unknown type of $lh")
        false
    }
}

private fun verifyProgress(mock: ProgressCallback, minPages: Int) =
    verify(mock, atLeast(minPages)).onProgressUpdate(
        argThat { done -> done in 0 until minPages },
        argThat { total -> total >= minPages })
