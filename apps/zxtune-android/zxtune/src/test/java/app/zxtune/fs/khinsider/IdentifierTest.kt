package app.zxtune.fs.khinsider

import androidx.core.net.toUri
import org.junit.Assert.assertEquals
import org.junit.Test
import org.junit.runner.RunWith
import org.robolectric.RobolectricTestRunner

@RunWith(RobolectricTestRunner::class)
class IdentifierTest {
    @Test
    fun `empty path`() {
        val uri = "khinsider:".toUri()
        requireNotNull(Identifier.find(uri)).run {
            assertEquals(null, category)
            assertEquals(null, scope)
            assertEquals(null, album)
            assertEquals(null, track)
            assertEquals(null, location)
            assertEquals(uri, toUri())
        }
    }

    @Test
    fun `only groupings`() = Identifier.Category.entries.forEach { group ->
        val uri = "khinsider:/${group.name}".toUri()
        if (group.isImage) {
            assertEquals(null, Identifier.find(uri))
        } else requireNotNull(Identifier.find(uri)).run {
            assertEquals(group, category)
            assertEquals(null, scope)
            assertEquals(null, album)
            assertEquals(null, track)
            assertEquals(null, location)
            assertEquals(uri, toUri())
        }
    }

    @Test
    fun `with scope`() {
        val uri = "khinsider:/Top/TopType?scope=top%2Fscope".toUri()
        requireNotNull(Identifier.find(uri)).run {
            assertEquals(Identifier.Category.Top, category)
            assertEquals(makeScope("top/scope", "TopType"), scope)
            assertEquals(null, album)
            assertEquals(null, track)
            assertEquals(null, location)
            assertEquals(uri, toUri())
        }
    }

    @Test
    fun `with album`() {
        val uri = "khinsider:/Series/SeriesName/AlbumName?scope=seriesid&album=albumid".toUri()
        requireNotNull(Identifier.find(uri)).run {
            assertEquals(Identifier.Category.Series, category)
            assertEquals(makeScope("seriesid", "SeriesName"), scope)
            assertEquals(makeAlbum("albumid", "AlbumName"), album)
            assertEquals(null, track)
            assertEquals(null, location)
            assertEquals(uri, toUri())
        }
    }

    @Test
    fun `with track`() {
        val uri =
            "khinsider:/AlbumsByLetter/S/SomeAlbum/TrackName?scope=albums%2Fby-letter%2Fs&album=some-album&track=TrackId.mp3".toUri()
        requireNotNull(Identifier.find(uri)).run {
            assertEquals(Identifier.Category.AlbumsByLetter, category)
            assertEquals(makeScope("albums/by-letter/s", "S"), scope)
            assertEquals(makeAlbum("some-album", "SomeAlbum"), album)
            assertEquals(makeTrack("TrackId.mp3", "TrackName"), track)
            assertEquals(null, location)
            assertEquals(uri, toUri())
        }
    }

    @Test
    fun `random category`() {
        val uri = "khinsider:/Random".toUri()
        requireNotNull(Identifier.find(uri)).run {
            assertEquals(Identifier.Category.Random, category)
            assertEquals(null, scope)
            assertEquals(null, album)
            assertEquals(null, track)
            assertEquals(null, location)
            assertEquals(uri, toUri())
        }
    }

    @Test
    fun `random album track`() {
        val uri = "khinsider:/Random/AlbumTitle/TrackTitle?album=albumid&track=Track.mp3".toUri()
        requireNotNull(Identifier.find(uri)).run {
            assertEquals(Identifier.Category.Random, category)
            assertEquals(null, scope)
            assertEquals(makeAlbum("albumid", "AlbumTitle"), album)
            assertEquals(makeTrack("Track.mp3", "TrackTitle"), track)
            assertEquals(null, location)
            assertEquals(uri, toUri())
        }
    }

    @Test
    fun `image urls`() {
        val image = "khinsider:/Image?location=scheme%3A%2Fa%2Fb".toUri()
        requireNotNull(Identifier.find(image)).run {
            assertEquals(Identifier.Category.Image, category)
            assertEquals(null, scope)
            assertEquals(null, album)
            assertEquals(null, track)
            assertEquals("scheme:/a/b".toUri(), location?.value)
            assertEquals(image, toUri())
        }
        val thumb = "khinsider:/Thumb?location=scheme%3A%2Fc%2Fd".toUri()
        requireNotNull(Identifier.find(thumb)).run {
            assertEquals(Identifier.Category.Thumb, category)
            assertEquals(null, scope)
            assertEquals(null, album)
            assertEquals(null, track)
            assertEquals("scheme:/c/d".toUri(), location?.value)
            assertEquals(thumb, toUri())
        }
    }

    @Test
    fun `invalid urls`() {
        assertEquals(null, Identifier.find("scheme:/".toUri()))
        requireNotNull(Identifier.find("khinsider:/WrongGrouping".toUri())).run {
            assertEquals(null, category)
            assertEquals(null, scope)
            assertEquals(null, album)
            assertEquals(null, track)
            assertEquals(null, location)
            assertEquals("khinsider:".toUri(), toUri())
        }
        requireNotNull(Identifier.find("khinsider:/Top/NoScope".toUri())).run {
            assertEquals(Identifier.Category.Top, category)
            assertEquals(null, scope)
            assertEquals(null, album)
            assertEquals(null, track)
            assertEquals(null, location)
            assertEquals("khinsider:/Top".toUri(), toUri())
        }
        requireNotNull(Identifier.find("khinsider:/Top/Scope/NoAlbum?scope=scopeid".toUri())).run {
            assertEquals(Identifier.Category.Top, category)
            assertEquals(makeScope("scopeid", "Scope"), scope)
            assertEquals(null, album)
            assertEquals(null, track)
            assertEquals(null, location)
            assertEquals("khinsider:/Top/Scope?scope=scopeid".toUri(), toUri())
        }
        requireNotNull(Identifier.find("khinsider:/Top/Scope/Album/NoTrack?scope=scopeid&album=albumid".toUri())).run {
            assertEquals(Identifier.Category.Top, category)
            assertEquals(makeScope("scopeid", "Scope"), scope)
            assertEquals(makeAlbum("albumid", "Album"), album)
            assertEquals(null, track)
            assertEquals(null, location)
            assertEquals("khinsider:/Top/Scope/Album?scope=scopeid&album=albumid".toUri(), toUri())
        }
    }
}

private fun makeScope(id: String, title: String) = Scope(Scope.Id(id), title)
private fun makeAlbum(id: String, title: String) = Album(Album.Id(id), title)
private fun makeTrack(id: String, title: String) = Track(Track.Id(id), title)
