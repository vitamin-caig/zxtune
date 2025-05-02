package app.zxtune.fs.ocremix

import android.net.Uri
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class IdentifierTest {
    @Test
    fun `empty path`() = Uri.parse("ocremix:").let { uri ->
        val chain = Identifier.findElementsChain(uri)
        assertEquals(0, chain.size)
        assertEquals(uri, Identifier.forElementsChain(chain))
    }

    @Test
    fun `root aggregate`() = Uri.parse("ocremix:/Systems").let { uri ->
        val chain = Identifier.findElementsChain(uri)
        assertEquals(1, chain.size)
        assertEquals("/Systems", serialize(chain))
        assertEquals(uri, Identifier.forElementsChain(chain))
    }

    @Test
    fun `system under root`() = Uri.parse("ocremix:/Organizations/MicroSoft?org=ms").let { uri ->
        val chain = Identifier.findElementsChain(uri)
        assertEquals(2, chain.size)
        assertEquals("/Organizations/MicroSoft[org=ms]", serialize(chain))
        assertEquals(uri, Identifier.forElementsChain(chain))
    }

    @Test
    fun `root game albums`() = Uri.parse("ocremix:/Games/TopGame/Albums?game=top").let { uri ->
        val chain = Identifier.findElementsChain(uri)
        assertEquals(3, chain.size)
        assertEquals("/Games/TopGame[game=top]/Albums", serialize(chain))
        assertEquals(uri, Identifier.forElementsChain(chain))
    }

    @Test
    fun `root remix`() = Uri.parse("ocremix:/Remixes/TopRemix?remix=top").let { uri ->
        val chain = Identifier.findElementsChain(uri)
        assertEquals(2, chain.size)
        assertEquals("/Remixes/TopRemix[remix=top]", serialize(chain))
        assertEquals(uri, Identifier.forElementsChain(chain))
    }

    @Test
    fun `game chiptune`() =
        Uri.parse("ocremix:/Systems/Sys/Games/Gm/chip.tune?sys=s&game=g&file=path%2Fto%2Fchip.tune.ext")
            .let { uri ->
                val chain = Identifier.findElementsChain(uri)
                assertEquals(5, chain.size)
                assertEquals(
                    "/Systems/Sys[sys=s]/Games/Gm[game=g]/chip.tune[file=path/to/chip.tune.ext]",
                    serialize(chain)
                )
                assertEquals(uri, Identifier.forElementsChain(chain))
            }

    @Test
    fun `game album track`() {
        Uri.parse("ocremix:/Organizations/Org/Games/Ga/Albums/Al/Track?org=o&game=g&album=a&file=path%2Fto%2FTrack.mp3")
            .let { uri ->
                val chain = Identifier.findElementsChain(uri)
                assertEquals(7, chain.size)
                assertEquals(
                    "/Organizations/Org[org=o]/Games/Ga[game=g]/Albums/Al[album=a]/Track[file=path/to/Track.mp3]",
                    serialize(chain)
                )
                assertEquals(uri, Identifier.forElementsChain(chain))
            }
    }

    @Test
    fun `failed to parse`() {
        assertEquals(
            0, Identifier.findElementsChain(Uri.parse("ocremix:/Organizations/Game?game=g")).size
        )
        assertEquals(
            0,
            Identifier.findElementsChain(Uri.parse("ocremix:/Games/Game/Albums/Al/NoFile?game=g&album=a")).size
        )
    }

    @Test
    fun `pictures paths`() {
        Uri.parse("ocremix:/Image?file=path%2Fto%2Fimage.png").let { uri ->
            val chain = Identifier.findElementsChain(uri)
            assertEquals(1, chain.size)
            assertEquals("/Image[file=path/to/image.png]", serialize(chain))
            (chain.last() as Identifier.PictureElement).run {
                assertEquals(Identifier.PictureElement.Type.Image, type)
                assertEquals("path/to/image.png", path.value)
                assertEquals(uri, Identifier.forImage(path))
            }
        }
        Uri.parse("ocremix:/Thumb?file=path%2Fto%2Fthumb.jpg").let { uri ->
            val chain = Identifier.findElementsChain(uri)
            assertEquals(1, chain.size)
            assertEquals("/Thumb[file=path/to/thumb.jpg]", serialize(chain))
            (chain.last() as Identifier.PictureElement).run {
                assertEquals(Identifier.PictureElement.Type.Thumb, type)
                assertEquals("path/to/thumb.jpg", path.value)
                assertEquals(uri, Identifier.forThumb(path))
            }
        }
    }
}

private fun serialize(chain: Array<Identifier.PathElement>) = StringBuilder().apply {
    chain.forEach {
        append('/')
        append(it.title)
        it.key?.let { (key, value) ->
            append("[$key=$value]")
        }
    }
}.toString()