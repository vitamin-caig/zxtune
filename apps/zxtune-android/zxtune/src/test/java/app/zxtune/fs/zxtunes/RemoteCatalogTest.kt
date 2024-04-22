package app.zxtune.fs.zxtunes

import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import org.junit.Assert
import org.junit.Assert.assertEquals
import org.junit.Assert.assertNull
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.mock
import org.mockito.kotlin.verify
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class RemoteCatalogTest {

    private val author5 = Author(5, "Zdw", "Dmitriy Zubarev", false)
    private val author302 = Author(302, "(R)Soft", "Vladimir Bakum", true)
    private val author519 = Author(519, "MmcM", "Sergey Kosov", true)
    private val author1063 = Author(1063, "Nooly", "Vojta Nedved", true)

    private lateinit var catalog: RemoteCatalog

    @Before
    fun setUp() = with(HttpProviderFactory.createTestProvider()) {
        catalog = RemoteCatalog(MultisourceHttpProvider(this))
    }

    @Test
    fun `test queryAuthors`() {
        val visitor = mock<Catalog.AuthorsVisitor>()

        // http://zxtunes.com/xml.php?scope=authors&fields=nickname,name,tracks
        catalog.queryAuthors(visitor)

        // no count hint
        //verify(visitor).setCountHint(argThat { hint -> hint > 900 })
        verify(visitor).accept(author5)
        verify(visitor).accept(author302)
        verify(visitor).accept(author519)
        verify(visitor).accept(author1063)
    }

    @Test
    fun `test queryAuthor`() {
        assertNull(catalog.queryAuthor(-1))
        for (test in arrayOf(author5, author302, author519, author1063)) {
            assertEquals(test, requireNotNull(catalog.queryAuthor(test.id)))
        }
    }

    @Test
    fun `test queryAuthorTracks`() {
        val visitor = mock<Catalog.TracksVisitor>()

        // http://zxtunes.com/xml.php?scope=tracks&fields=filename,title,duration,date&author_id=483
        catalog.queryAuthorTracks(Author(483, "UNUSED", "UNUSED"), visitor)

        // no count hint
        // verify(visitor).setCountHint(argThat { hint -> hint > 240 })
        verify(visitor).accept(
            Track(
                13031,
                "fable.pt2",
                "FABLE..........R.MILES+KLIM'96",
                10560,
                1996
            )
        )
        // no date
        verify(visitor).accept(
            Track(
                13151,
                "logo.pt3",
                "Klim/OHG/PZS - Omega Hackers Group logo",
                320
            )
        )
        // symbols
        verify(visitor).accept(
            Track(
                13100,
                "Klim_65.pt3",
                "AMIGA 1200 & KLIM/OHG/MAFIA'98 - <<<<<<<<LEPRIKON-1>>>>>>>>>>>>>>",
                4112,
                1998
            )
        )
    }

    @Test
    fun `test getTrackUris()`() = with(RemoteCatalog.getTrackUris(12345)) {
        Assert.assertEquals(1L, size.toLong())
        Assert.assertEquals("http://zxtunes.com/downloads.php?id=12345", get(0).toString())
    }

    @Test
    fun `test getImageUris()`() = with(RemoteCatalog.getImageUris(12345)) {
        Assert.assertEquals(1L, size.toLong())
        Assert.assertEquals("http://zxtunes.com/photo/12345.jpg", get(0).toString())
    }
}
