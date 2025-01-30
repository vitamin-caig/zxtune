package app.zxtune.fs.ocremix

data class System(val id: Id, val title: String) {
    @JvmInline
    value class Id(val value: String)
}

data class Organization(val id: Id, val title: String) {
    @JvmInline
    value class Id(val value: String)
}

data class Game(val id: Id, val title: String) {
    @JvmInline
    value class Id(val value: String)

    data class Details(val chiptunePath: FilePath?, val image: FilePath?)
}

// Stripped /files/ part due to chiptune path format on game's page
// music/chiptunes/archives/${letter}/${filename}
// music/remixes/${filename}
// torrents/${filename}
// albums/${folder}/${path}
@JvmInline
value class FilePath(val value: String) {
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
    @JvmInline
    value class Id(val value: String)
}

data class Album(val id: Id, val title: String) {
    @JvmInline
    value class Id(val value: String)
}
