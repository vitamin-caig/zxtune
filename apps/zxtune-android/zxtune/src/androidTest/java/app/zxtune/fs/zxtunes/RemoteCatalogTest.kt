package app.zxtune.fs.zxtunes

import androidx.test.ext.junit.runners.AndroidJUnit4
import app.zxtune.fs.http.HttpProviderFactory
import app.zxtune.fs.http.MultisourceHttpProvider
import org.junit.Before
import org.junit.Test
import org.junit.runner.RunWith
import org.mockito.kotlin.mock
import org.mockito.kotlin.verify

@RunWith(AndroidJUnit4::class)
class RemoteCatalogTest {

    private lateinit var catalog: RemoteCatalog

    @Before
    fun setUp() = with(HttpProviderFactory.createTestProvider()) {
        catalog = RemoteCatalog(MultisourceHttpProvider(this))
    }

    @Test
    fun test_queryAuthors() {
        val visitor = mock<Catalog.AuthorsVisitor>()

        // http://zxtunes.com/xml.php?scope=authors&fields=nickname,name,tracks
        catalog.queryAuthors(visitor)

        // no count hint
        //verify(visitor).setCountHint(argThat { hint -> hint > 900 })
        verify(visitor).accept(Author(302, "(R)Soft", "Vladimir Bakum"))
        verify(visitor).accept(Author(519, "MmcM", "Sergey Kosov"))
        verify(visitor).accept(Author(1063, "Nooly", "Vojta Nedved"))
    }

    @Test
    fun test_queryAuthorTracks() {
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
}
