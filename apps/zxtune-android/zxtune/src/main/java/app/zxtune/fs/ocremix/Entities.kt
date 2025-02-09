package app.zxtune.fs.ocremix

// TODO: inline classes are supported since room 2.6.0-alpha01, so dao can read direct entities

data class System(val id: Id, val title: String) {
    data class Id(val value: String)
}

data class Organization(val id: Id, val title: String) {
    data class Id(val value: String)
}

data class Game(val id: Id, val title: String) {
    data class Id(val value: String)

    data class Details(val chiptunePath: FilePath?, val image: FilePath?)
}

// Stripped /files/ part due to chiptune path format on game's page
// music/chiptunes/archives/${letter}/${filename}
// music/remixes/${filename}
// torrents/${filename}
// albums/${folder}/${path}
data class FilePath(val value: String) {
    init {
        require(value.isNotEmpty() && '/' != value.first())
    }

    val displayName
        get() = value.substringAfterLast('/').substringBeforeLast('.')

    val isMusic
        get() = value.endsWith(".mp3", ignoreCase = true)

    val isTorrent
        get() = value.endsWith(".torrent", ignoreCase = true)
}

data class Remix(val id: Id, val title: String) {
    data class Id(val value: String)
}

data class Album(val id: Id, val title: String) {
    data class Id(val value: String)
}
