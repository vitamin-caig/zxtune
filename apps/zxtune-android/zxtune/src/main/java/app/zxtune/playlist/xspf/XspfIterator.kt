/**
 *
 * @file
 *
 * @brief Xspf file parser
 *
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.playlist.xspf

import android.net.Uri
import android.sax.RootElement
import android.sax.TextElementListener
import app.zxtune.TimeStamp
import app.zxtune.io.Io.createByteBufferInputStream
import app.zxtune.playlist.ReferencesArrayIterator
import app.zxtune.playlist.ReferencesIterator
import app.zxtune.utils.Xml
import java.io.IOException
import java.io.InputStream
import java.nio.ByteBuffer

object XspfIterator {
    @Throws(IOException::class)
    @JvmStatic
    fun create(buf: ByteBuffer): ReferencesIterator {
        return ReferencesArrayIterator(parse(buf))
    }

    @Throws(IOException::class)
    @JvmStatic
    fun parse(buf: ByteBuffer): ArrayList<ReferencesIterator.Entry> {
        return parse(createByteBufferInputStream(buf))
    }

    @Throws(IOException::class)
    @JvmStatic
    fun parse(stream: InputStream): ArrayList<ReferencesIterator.Entry> =
        ArrayList<ReferencesIterator.Entry>().apply {
            Xml.parse(stream, createPlaylistParseRoot(this).contentHandler)
        }
}

private fun createPlaylistParseRoot(entries: ArrayList<ReferencesIterator.Entry>) =
    RootElement(Meta.XMLNS, Tags.PLAYLIST).apply {
        val builder = EntriesBuilder()
        //TODO: check extension
        getChild(Meta.XMLNS, Tags.EXTENSION).getChild(Meta.XMLNS, Tags.PROPERTY)
            .setTextElementListener(object : TextElementListener {
                private var propName: String? = null
                override fun start(attributes: org.xml.sax.Attributes) {
                    propName = attributes.getValue(Attributes.NAME)
                }

                override fun end(body: String) {
                    if (Properties.PLAYLIST_CREATOR == propName) {
                        builder.desktopPaths = body.startsWith("zxtune-qt")
                    } else if (Properties.PLAYLIST_VERSION == propName) {
                        builder.escapedTexts = body.toInt() < 2
                    }
                }
            })
        with(getChild(Meta.XMLNS, Tags.TRACKLIST).getChild(Meta.XMLNS, Tags.TRACK)) {
            setEndElementListener {
                builder.captureResult()?.let {
                    entries.add(it)
                }
            }
            getChild(Meta.XMLNS, Tags.LOCATION).setEndTextElementListener {
                it?.let { builder.setLocation(it) }
            }
            getChild(Meta.XMLNS, Tags.TITLE).setEndTextElementListener {
                it?.let { builder.setTitle(it) }
            }
            getChild(Meta.XMLNS, Tags.CREATOR).setEndTextElementListener {
                it?.let { builder.setCreator(it) }
            }
            getChild(Meta.XMLNS, Tags.DURATION).setEndTextElementListener {
                it?.let { builder.setDuration(it) }
            }
            //TODO: parse rest properties
        }
    }

private class EntriesBuilder {

    private var result: ReferencesIterator.Entry = ReferencesIterator.Entry()
    var desktopPaths = false
    var escapedTexts = true

    fun setLocation(location: String) {
        result.location = if (desktopPaths) {
            location //subpath for ay/sid containers
                .replace("?#", "#%23") //rest paths with subpath
                .replace('?', '#')
        } else {
            location
        }
    }

    fun setTitle(title: String) {
        result.title = decode(title)
    }

    fun setCreator(author: String) {
        result.author = decode(author)
    }

    fun setDuration(duration: String) = duration.toLongOrNull()?.let { ms ->
        result.duration = TimeStamp.fromMilliseconds(ms)
    }

    private fun decode(str: String) = if (escapedTexts) Uri.decode(str) else str

    fun captureResult() = result.location?.let {
        result
    }.also {
        result = ReferencesIterator.Entry()
    }
}
