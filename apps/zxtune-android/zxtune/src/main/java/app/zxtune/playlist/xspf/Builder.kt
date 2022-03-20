package app.zxtune.playlist.xspf

import android.util.Xml
import app.zxtune.playlist.Item
import org.xmlpull.v1.XmlSerializer
import java.io.IOException
import java.io.OutputStream

internal class Builder(output: OutputStream) {
    private val xml: XmlSerializer = Xml.newSerializer()
    private var hasTracklist = false

    init {
        xml.setOutput(output, Meta.ENCODING)
        xml.setFeature("http://xmlpull.org/v1/doc/features.html#indent-output", true)
        xml.startDocument(Meta.ENCODING, true)
        xml.startTag(null, Tags.PLAYLIST)
            .attribute(null, Attributes.VERSION, Meta.XSPF_VERSION)
            .attribute(null, Attributes.XMLNS, Meta.XMLNS)
    }

    @Throws(IOException::class)
    fun finish() {
        if (hasTracklist) {
            xml.endTag(null, Tags.TRACKLIST)
        }
        xml.endTag(null, Tags.PLAYLIST)
        xml.endDocument()
        xml.flush()
    }

    @Throws(IOException::class)
    fun writePlaylistProperties(name: String, items: Int?) {
        xml.startTag(null, Tags.EXTENSION)
            .attribute(null, Attributes.APPLICATION, Meta.APPLICATION)
        writeProperty(Properties.PLAYLIST_VERSION, Meta.VERSION.toString())
        writeProperty(Properties.PLAYLIST_NAME, name)
        items?.run {
            writeProperty(Properties.PLAYLIST_ITEMS, toString())
        }
        xml.endTag(null, Tags.EXTENSION)
    }

    @Throws(IOException::class)
    fun writeTrack(item: Item) {
        if (!hasTracklist) {
            xml.startTag(null, Tags.TRACKLIST)
            hasTracklist = true
        }
        xml.startTag(null, Tags.TRACK)
        writeTag(Tags.LOCATION, item.location.toString())
        writeTag(Tags.CREATOR, item.author)
        writeTag(Tags.TITLE, item.title)
        writeTag(Tags.DURATION, item.duration.toMilliseconds().toString())
        //TODO: save extended properties
        xml.endTag(null, Tags.TRACK)
    }

    private fun writeProperty(name: String, value: String) =
        xml.startTag(null, Tags.PROPERTY)
            .attribute(null, Attributes.NAME, name)
            .text(value)
            .endTag(null, Tags.PROPERTY)

    private fun writeTag(name: String, value: String) {
        if (value.isNotEmpty()) {
            xml.startTag(null, name).text(value).endTag(null, name)
        }
    }
}