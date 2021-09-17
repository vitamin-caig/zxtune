/**
 * @file
 * @brief Remote implementation of catalog
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.zxtunes

import android.net.Uri
import android.sax.RootElement
import android.util.Xml
import app.zxtune.Logger
import app.zxtune.fs.HtmlUtils.tryGetInteger
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.ifNotNulls
import org.xml.sax.Attributes
import org.xml.sax.SAXException
import java.io.BufferedInputStream
import java.io.IOException

private val LOG = Logger(RemoteCatalog::class.java.name)

class RemoteCatalog internal constructor(private val http: MultisourceHttpProvider) : Catalog {
    override fun queryAuthors(visitor: Catalog.AuthorsVisitor) {
        LOG.d { "queryAuthors()" }
        val root = createAuthorsParserRoot(visitor)
        performQuery(Uris.forAllAuthors(), root)
    }

    override fun queryAuthorTracks(author: Author, visitor: Catalog.TracksVisitor) {
        LOG.d { "queryAuthorTracks(author=${author.id})" }
        queryTracks(visitor, Uris.forTracks(author))
    }

    private fun queryTracks(visitor: Catalog.TracksVisitor, uri: Uri) {
        val root = createModulesParserRoot(visitor)
        performQuery(uri, root)
    }

    override fun searchSupported() = http.hasConnection()

    override fun findTracks(query: String, visitor: Catalog.FoundTracksVisitor) =
        throw IOException("Search is not supported on remote side")

    private fun performQuery(uri: Uri, root: RootElement) = try {
        BufferedInputStream(http.getInputStream(uri)).use { stream ->
            Xml.parse(stream, Xml.Encoding.UTF_8, root.contentHandler)
        }
    } catch (e: SAXException) {
        throw IOException(e)
    }

    companion object {
        @JvmStatic
        fun getTrackUris(id: Int) = arrayOf(Uris.forDownload(id))
    }
}

private class Uris {
    private val delegate = Uri.Builder()

    init {
        delegate.scheme("http") //TODO: https
            .authority("zxtunes.com")
    }

    fun forApi(): Uri = delegate.appendPath("xml.php").build()

    fun forDownloads(): Uri = delegate.appendPath("downloads.php").build()

    fun scope(scope: String) = apply {
        delegate.appendQueryParameter("scope", scope)
    }

    fun fields(fields: String) = apply {
        delegate.appendQueryParameter("fields", fields)
    }

    fun entity(entry: String, id: Int) = apply {
        delegate.appendQueryParameter(entry, id.toString())
    }

    companion object {
        fun forAllAuthors() =
            Uris().scope("authors").fields("nickname,name,tracks").forApi()

        fun forTracks(author: Author) =
            Uris().scope("tracks").fields("filename,title,duration,date")
                .entity("author_id", author.id).forApi()

        fun forDownload(id: Int) = Uris().entity("id", id).forDownloads()
    }
}

private class AuthorBuilder {
    private var id: Int? = null
    private var nickname: String? = null
    private var name = ""
    private var tracks: Int = 0

    init {
        reset()
    }

    fun setId(value: String?) {
        id = tryGetInteger(value)
    }

    fun setNickname(value: String?) {
        nickname = value
    }

    fun setName(value: String?) {
        name = value.orEmpty()
    }

    fun setTracks(value: String?) {
        tracks = tryGetInteger(value) ?: 0
    }

    fun captureResult() = ifNotNulls(id, nickname, name.takeIf { tracks > 0 }, ::Author).also {
        reset()
    }

    private fun reset() {
        id = null
        nickname = null
        tracks = 0
        name = ""
    }
}

private class ModuleBuilder {
    private var id: Int? = null
    private var filename: String? = null
    private var title: String = ""
    private var duration: Int? = null
    private var date: Int? = null

    init {
        reset()
    }

    fun setId(value: String?) {
        id = tryGetInteger(value)
    }

    fun setFilename(value: String?) {
        filename = value
    }

    fun setTitle(value: String?) {
        title = value.orEmpty()
    }

    fun setDuration(value: String?) {
        duration = tryGetInteger(value)
    }

    fun setDate(value: String?) {
        date = tryGetInteger(value)
    }

    fun captureResult() = ifNotNulls(id, filename) { id, filename ->
        Track(id, filename, title, duration, date)
    }.also {
        reset()
    }

    private fun reset() {
        id = null
        duration = null
        date = null
        filename = null
        title = ""
    }
}

private fun createAuthorsParserRoot(visitor: Catalog.AuthorsVisitor) = createRootElement().apply {
    getChild("authors").run {
        setStartElementListener { attributes: Attributes ->
            tryGetInteger(attributes.getValue("count"))?.let { count ->
                visitor.setCountHint(count)
            }
        }
        val builder = AuthorBuilder()
        getChild("author").run {
            setStartElementListener { attributes: Attributes ->
                builder.setId(attributes.getValue("id"))
            }
            setEndElementListener {
                builder.captureResult()?.let {
                    visitor.accept(it)
                }
            }
            getChild("nickname").setEndTextElementListener(builder::setNickname)
            getChild("name").setEndTextElementListener(builder::setName)
            getChild("tracks").setEndTextElementListener(builder::setTracks)
        }
    }
}

private fun createModulesParserRoot(visitor: Catalog.TracksVisitor) = createRootElement().apply {
    val builder = ModuleBuilder()
    getChild("tracks").run {
        setStartElementListener { attributes: Attributes ->
            tryGetInteger(attributes.getValue("count"))?.let { count ->
                visitor.setCountHint(count)
            }
        }
        getChild("track").run {
            setStartElementListener { attributes: Attributes ->
                builder.setId(attributes.getValue("id"))
            }
            setEndElementListener {
                builder.captureResult()?.let {
                    visitor.accept(it)
                }
            }
            getChild("filename").setEndTextElementListener(builder::setFilename)
            getChild("title").setEndTextElementListener(builder::setTitle)
            getChild("duration").setEndTextElementListener(builder::setDuration)
            getChild("date").setEndTextElementListener(builder::setDate)
        }
    }
}

//TODO: check root tag version
private fun createRootElement() = RootElement("zxtunes")

