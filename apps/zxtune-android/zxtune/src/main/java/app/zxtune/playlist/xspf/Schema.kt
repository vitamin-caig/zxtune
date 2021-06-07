package app.zxtune.playlist.xspf

internal object Attributes {
    const val VERSION = "version"
    const val XMLNS = "xmlns"
    const val APPLICATION = "application"
    const val NAME = "name"
}

internal object Meta {
    const val ENCODING = "UTF-8"
    const val XMLNS = "http://xspf.org/ns/0/"
    const val APPLICATION = "http://zxtune.googlecode.com"
    const val XSPF_VERSION = "1"

    // V2 does not escape string values
    const val VERSION = 2
}

internal object Properties {
    const val PLAYLIST_CREATOR = "zxtune.app.playlist.creator"
    const val PLAYLIST_NAME = "zxtune.app.playlist.name"
    const val PLAYLIST_VERSION = "zxtune.app.playlist.version"
    const val PLAYLIST_ITEMS = "zxtune.app.playlist.items"
}

internal object Tags {
    const val PLAYLIST = "playlist"
    const val EXTENSION = "extension"
    const val PROPERTY = "property"
    const val TRACKLIST = "trackList"
    const val TRACK = "track"
    const val LOCATION = "location"
    const val CREATOR = "creator"
    const val TITLE = "title"
    const val DURATION = "duration"
}
