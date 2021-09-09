package app.zxtune.utils

import org.xml.sax.ContentHandler
import org.xml.sax.InputSource
import org.xml.sax.SAXException
import java.io.IOException
import java.io.InputStream
import javax.xml.parsers.SAXParserFactory

object Xml {

    private val factory = SAXParserFactory.newInstance()

    fun parse(input: InputStream, handler: ContentHandler) = try {
        val reader = factory.newSAXParser().xmlReader
        reader.setFeature("http://xml.org/sax/features/namespaces", true)
        reader.contentHandler = handler
        input.use {
            reader.parse(InputSource(it))
        }
    } catch (e: SAXException) {
        throw IOException(e)
    }
}
