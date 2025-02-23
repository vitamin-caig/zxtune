/**
 * @file
 * @brief Remote implementation of catalog
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.zxtunes

import android.net.Uri
import android.sax.RootElement
import app.zxtune.Logger
import app.zxtune.fs.HtmlUtils.tryGetInteger
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.utils.Xml
import app.zxtune.utils.ifNotNulls
import org.xml.sax.Attributes
import java.io.BufferedInputStream
import java.io.IOException

private val LOG = Logger(RemoteCatalog::class.java.name)

class RemoteCatalog internal constructor(private val http: MultisourceHttpProvider) : Catalog {
    override fun queryAuthors(visitor: Catalog.Visitor<Author>) {
        LOG.d { "queryAuthors()" }
        val root = createAuthorsParserRoot(visitor)
        performQuery(Uris.forAllAuthors(), root)
    }

    override fun queryAuthor(id: Int): Author? {
        LOG.d { "queryAuthor(id=${id})" }
        val visitor = object : Catalog.Visitor<Author> {
            var result: Author? = null
            override fun accept(obj: Author) {
                require(result == null)
                result = obj
            }
        }
        val root = createAuthorsParserRoot(visitor)
        performQuery(Uris.forAuthor(id), root)
        return visitor.result
    }

    override fun queryAuthorTracks(author: Author, visitor: Catalog.Visitor<Track>) {
        LOG.d { "queryAuthorTracks(author=${author.id})" }
        queryTracks(visitor, Uris.forTracks(author))
    }

    private fun queryTracks(visitor: Catalog.Visitor<Track>, uri: Uri) {
        val root = createModulesParserRoot(visitor)
        performQuery(uri, root)
    }

    override fun searchSupported() = http.hasConnection()

    override fun findTracks(query: String, visitor: Catalog.FoundTracksVisitor) =
        throw IOException("Search is not supported on remote side")

    private fun performQuery(uri: Uri, root: RootElement) =
        http.getInputStream(uri).let { httpStream ->
            Xml.parse(BufferedInputStream(httpStream), root.contentHandler)
        }

    companion object {
        @JvmStatic
        fun getTrackUris(id: Int) = arrayOf(Uris.forDownload(id))

        @JvmStatic
        fun getImageUris(id: Int) = arrayOf(Uris.forPhoto(id))
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

    fun forPhoto(id: Int): Uri = delegate.appendPath("photo").appendEncodedPath("${id}.jpg").build()

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
        private fun forAuthors() = Uris().scope("authors").fields("nickname,name,tracks,photo")

        fun forAllAuthors() = forAuthors().forApi()

        fun forAuthor(id: Int) = forAuthors().entity("id", id).forApi()

        fun forTracks(author: Author) =
            Uris().scope("tracks").fields("filename,title,duration,date")
                .entity("author_id", author.id).forApi()

        fun forDownload(id: Int) = Uris().entity("id", id).forDownloads()

        fun forPhoto(id: Int) = Uris().forPhoto(id)
    }
}

private class AuthorBuilder {
    private var id: Int? = null
    private var nickname: String? = null
    private var name = ""
    private var tracks: Int = 0
    private var hasPhoto = false

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

    fun setHasPhoto(value: String?) {
        hasPhoto = 1 == tryGetInteger(value)
    }

    fun captureResult() =
        ifNotNulls(id, nickname, name.takeIf { tracks > 0 }, hasPhoto, ::Author).also {
            reset()
        }

    private fun reset() {
        id = null
        nickname = null
        tracks = 0
        name = ""
        hasPhoto = false
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

private fun createAuthorsParserRoot(visitor: Catalog.Visitor<Author>) = createRootElement().apply {
    getChild("authors").run {
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
            getChild("photo").setEndTextElementListener(builder::setHasPhoto)
        }
    }
}

private fun createModulesParserRoot(visitor: Catalog.Visitor<Track>) = createRootElement().apply {
    val builder = ModuleBuilder()
    getChild("tracks").getChild("track").run {
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

//TODO: check root tag version
private fun createRootElement() = RootElement("zxtunes")
