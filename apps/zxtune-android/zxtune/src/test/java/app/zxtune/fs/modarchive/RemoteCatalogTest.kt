package app.zxtune.fs.modarchive

import app.zxtune.BuildConfig
import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ProgressCallback
import org.junit.Assert.assertEquals
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.AdditionalMatchers.geq
import org.mockito.AdditionalMatchers.gt
import org.mockito.kotlin.any
import org.mockito.kotlin.argThat
import org.mockito.kotlin.atLeast
import org.mockito.kotlin.atLeastOnce
import org.mockito.kotlin.mock
import org.mockito.kotlin.never
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class RemoteCatalogTest {

    private lateinit var catalog: RemoteCatalog

    @Before
    fun setUp() = with(HttpProviderFactory.createTestProvider()) {
        catalog = RemoteCatalog(MultisourceHttpProvider(this), BuildConfig.MODARCHIVE_KEY)
    }

    @Test
    fun `test queryAuthors`() {
        val visitor = mock<Catalog.AuthorsVisitor>()
        val progress = mock<ProgressCallback>()
        val pagesMin = 65
        val authorsApprox = 2400
        catalog.queryAuthors(visitor, progress)
        verify(visitor, atLeast(pagesMin)).setCountHint(gt(authorsApprox))
        verify(visitor).accept(Author(68877, "_Matt_"))
        verify(visitor).accept(Author(83394, "4mat"))
        verify(visitor).accept(Author(0, "!Unknown"))
        verify(visitor).accept(Author(90081, "Дима"))
        verify(visitor, atLeast(authorsApprox)).accept(any())
        verify(progress, atLeast(pagesMin)).onProgressUpdate(geq(0), geq(pagesMin))
        verifyNoMoreInteractions(visitor, progress)
    }

    @Test
    fun `test queryGenres`() {
        val visitor = mock<Catalog.GenresVisitor>()
        val genresApprox = 75
        catalog.queryGenres(visitor)
        verify(visitor).setCountHint(gt(genresApprox))
        fun matchesGenre(id: Int, name: String, tracksMin: Int) = { genre: Genre ->
            genre.id == id && genre.name == name && genre.tracks >= tracksMin
        }
        verify(visitor).accept(argThat(matchesGenre(48, "Alternative", 440)))
        verify(visitor).accept(argThat(matchesGenre(71, "Trance (general)", 910)))
        verify(visitor).accept(argThat(matchesGenre(25, "Soul", 10)))
        verify(visitor, atLeast(genresApprox)).accept(any())
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test queryTracks by author`() {
        val visitor = mock<Catalog.TracksVisitor>()
        val progress = mock<ProgressCallback>()
        val tracksMin = 370
        val pagesMin = 10
        catalog.queryTracks(Author(83394, "UNUSED!!!"), visitor, progress)

        // No total tracks info, only on-page amount
        //verify(visitor).setCountHint(geq(trackMin))
        verify(visitor).accept(Track(182782, "4mat_-_broken_heart.xm", "<3 broken heart <3", 61760))
        verify(visitor).accept(Track(77970, "forest-loader.mod", "forest-loader", 24992))
        verify(visitor).accept(Track(140500, "egg_plant.xm", "<< EGG PLANT >>", 530975))
        verify(visitor, atLeast(tracksMin)).accept(any())
        verify(progress, atLeast(pagesMin)).onProgressUpdate(geq(0), geq(pagesMin))
        verifyNoMoreInteractions(visitor, progress)
    }

    @Test
    fun `test queryTracks by genre`() {
        val visitor = mock<Catalog.TracksVisitor>()
        val progress = mock<ProgressCallback>()
        val tracksMin = 440
        val pagesMin = 10
        catalog.queryTracks(Genre(48, "UNUSED!!!", -100500), visitor, progress)

        verify(visitor).accept(Track(94399, "_nice_outfit_.mod", "(.nice outfit.)", 43386))
        verify(visitor).accept(
            Track(
                190055, "joseph_tek_fox_-_ci_towermpa.s3m", "Tower of " + "Masamune Past", 235184
            )
        )
        verify(visitor).accept(Track(162204, "zulualert.mod", "zulu alert 3", 196092))
        verify(visitor, atLeast(tracksMin)).accept(any())
        verify(progress, atLeast(pagesMin)).onProgressUpdate(geq(0), geq(pagesMin))
        verifyNoMoreInteractions(visitor, progress)
    }

    @Test
    fun `test findTracks`() {
        val visitor = mock<Catalog.FoundTracksVisitor>()
        val minTracks = 260
        catalog.findTracks("test", visitor)
        verify(visitor).accept(
            Author(69008, "xtd"), Track(121407, "_chipek_test_.mod", "## chipek test ##", 13740)
        )
        verify(visitor).accept(
            Author(0, "!Unknown"), Track(49744, "ncontest.s3m", "no contest", 124440)
        )
        verify(visitor).accept(
            Author(0, "!Unknown"), Track(111269, "wildtest.mod", "wildtest", 2932)
        )
        verify(visitor, atLeast(minTracks)).accept(any(), any())
        verifyNoMoreInteractions(visitor)
    }

    @Test
    fun `test findRandomTracks`() {
        val visitor = mock<Catalog.TracksVisitor>()

        catalog.findRandomTracks(visitor)

        verify(visitor, never()).setCountHint(any())
        verify(visitor, atLeastOnce()).accept(any())
    }

    @Test
    fun `test getTrackUris`() = with(RemoteCatalog.getTrackUris(12345)) {
        assertEquals(2L, size.toLong())
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/modarchive/ids/12345", get(0).toString()
        )
        assertEquals(
            "https://api.modarchive.org/downloads.php?moduleid=12345", get(1).toString()
        )
    }
}
