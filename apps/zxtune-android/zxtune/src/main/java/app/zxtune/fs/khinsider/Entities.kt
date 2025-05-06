package app.zxtune.fs.khinsider

import android.net.Uri

// TODO: see app.zxtune.fs.ocremix.Entities notes

data class Scope(val id: Id, val title: String) {
    // Top:
    //  top40
    //  all-time-top-100 (!)
    //  last-6-months-top-100
    //  most-favorites
    // By letter:
    //  game-soundtracks/browse/A
    //  ..
    //  game-soundtracks/browse/0-9
    // By platform:
    //  game-soundtracks/nintendo-3ds
    // By type:
    //  game-soundtracks/gamerips
    // By year:
    //  game-soundtracks/year/2009
    // By series:
    //  kirby
    data class Id(val value: String) {
        init {
            require(value.isNotEmpty() && '/' != value.first())
        }
    }
}

data class Album(val id: Id, val title: String) {
    data class Id(val value: String)
}

data class AlbumAndDetails(val album: Album, val details: String, val image: FilePath?)

data class Track(
    val id: Id,
    val title: String,
) {
    // Filename, local to album
    data class Id(val value: String) {
        init {
            require(value.endsWith(suffix = ".mp3", ignoreCase = true))
        }
    }
}

data class TrackAndDetails(
    val album: Album, val track: Track, val index: Int, val duration: String, val size: String
)

// https://${host}/soundtracks/${Album.Id}/
//  Image.png|thumbs/Image.png
data class FilePath(val value: Uri) {
    init {
        require(value.isAbsolute && value != Uri.EMPTY)
    }

    val thumbUri
        get() = value.path?.run {
            require(!endsWith(".mp3"))
            val delimiter = lastIndexOf('/')
            substring(0, delimiter) + "/thumbs" + substring(delimiter)
        }?.let { fixedPath ->
            value.buildUpon().path(fixedPath).build()
        }
}
