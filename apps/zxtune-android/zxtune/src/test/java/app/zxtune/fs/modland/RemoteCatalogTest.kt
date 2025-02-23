package app.zxtune.fs.modland

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
import org.mockito.AdditionalMatchers.lt
import org.mockito.kotlin.any
import org.mockito.kotlin.argThat
import org.mockito.kotlin.atLeast
import org.mockito.kotlin.doAnswer
import org.mockito.kotlin.mock
import org.mockito.kotlin.times
import org.mockito.kotlin.verify
import org.mockito.kotlin.verifyNoMoreInteractions
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
        val visitor = mock<Catalog.Visitor<Group>>()
        val progress = mock<ProgressCallback>()
        val pagesMin = 3
        val groupsApprox = 95
        catalog.getAuthors().queryGroups("#", visitor, progress)

        verify(visitor).accept(argThat { matches(24209, "$4753 Softcopy", 33) })
        verify(visitor).accept(argThat { matches(22396, "9Nobo8", 1) })
        verify(visitor, atLeast(groupsApprox)).accept(any())
        verify(progress, atLeast(pagesMin)).onProgressUpdate(gt(0), geq(groupsApprox))
        verifyNoMoreInteractions(visitor, progress)
    }

    @Test
    fun `test authors tracks`() {
        val visitor = mock<Catalog.Visitor<Track>>()
        val progress = mock<ProgressCallback>()
        val minTracks = 700
        catalog.getAuthors().queryTracks(172 /*4-Mat*/, visitor, progress)

        verify(visitor).accept(
            Track(
                "/pub/modules/Nintendo%20SPC/4-Mat/Micro%20Machines/01%20-%20main%20theme.spc",
                66108
            )
        )
        verify(visitor).accept(Track("/pub/modules/Fasttracker%202/4-Mat/blade1.xm", 24504))
        verify(visitor, atLeast(minTracks)).accept(any())
        verify(progress, atLeast(minTracks / 40)).onProgressUpdate(any(), gt(minTracks))
        verifyNoMoreInteractions(visitor, progress)
    }

    @Test
    fun `test collections`() {
        val visitor = mock<Catalog.Visitor<Group>>()
        val progress = mock<ProgressCallback>()
        val pagesMin = 5
        val groupsApprox = 150
        catalog.getCollections().queryGroups("O", visitor, progress)

        verify(visitor).accept(argThat { matches(5307, "O&F", 10) })
        verify(visitor).accept(argThat { matches(5607, "Ozzyoss", 5) })
        verify(visitor, atLeast(groupsApprox)).accept(any())
        verify(progress, atLeast(pagesMin)).onProgressUpdate(
            gt(groupsApprox / pagesMin), gt(groupsApprox)
        )
        verifyNoMoreInteractions(visitor, progress)
    }

    @Test
    fun `test formats`() {
        val visitor = mock<Catalog.Visitor<Group>>()
        val progress = mock<ProgressCallback>()
        val pagesMin = 1
        val groupsApprox = 25
        catalog.getFormats().queryGroups("M", visitor, progress)
        verify(visitor).accept(argThat { matches(278, "Mad Tracker 2", 93) })
        verify(visitor).accept(argThat { matches(264, "MVX Module", 12) })
        verify(visitor, atLeast(groupsApprox)).accept(any())
        verify(progress, atLeast(pagesMin)).onProgressUpdate(
            gt(groupsApprox / pagesMin), gt(groupsApprox)
        )
        verifyNoMoreInteractions(visitor, progress)
    }

    @Test
    fun `test getTrackUris`() = with(RemoteCatalog.getTrackUris("/pub/modules/track.gz")) {
        assertEquals(2L, size.toLong())
        assertEquals(
            "${BuildConfig.CDN_ROOT}/download/modland/pub/modules/track.gz", get(0).toString()
        )
        assertEquals(
            "http://ftp.amigascne.org/mirrors/ftp.modland.com/pub/modules/track.gz",
            get(1).toString()
        )
    }
}

private fun Group.matches(id: Int, name: String, tracksMin: Int) =
    this.id == id && this.name == name && this.tracks >= tracksMin

