/**
 * @file
 * @brief Remote implementation of catalog
 * @author vitamin.caig@gmail.com
 */
package app.zxtune.fs.modarchive

import android.net.Uri
import android.sax.Element
import android.sax.RootElement
import android.text.Html
import app.zxtune.Logger
import app.zxtune.fs.HtmlUtils.tryGetInteger
import app.zxtune.fs.api.Cdn
import app.zxtune.fs.http.MultisourceHttpProvider
import app.zxtune.fs.modarchive.Catalog.*
import app.zxtune.utils.ProgressCallback
import app.zxtune.utils.StubProgressCallback
import app.zxtune.utils.Xml
import app.zxtune.utils.ifNotNulls
import java.io.BufferedInputStream

/**
 * API entry point:
 * http://api.modarchive.org
 *
 * Authors:
 * ${API}/xml-tools.php?key=${key}&request=search_artist&page=${page}
 *
 * references author's tracks at
 * ${API}/xml-tools.php?key=${key}&request=view_modules_by_artistid&query=${id}&page=${page}
 *
 * references tracks at
 * ${API}/downloads.php?moduleid=${moduleid}
 *
 *
 * Genres:
 * ${API}/xml-tools.php?key=${key}&request=view_genres (no paging)
 *
 * references genre's tracks at
 * ${API}/xml-tools.php?key=${key}&request=search&type=genre&query=${genreid}&page=${page}
 *
 *
 * Search:
 * ${API}/xml-tools.php?key=${key}&request=search&type=filename_or_songtitle&query=*${query}*&page=${page}
 */

private val LOG = Logger(RemoteCatalog::class.java.name)

open class RemoteCatalog internal constructor(
    private val http: MultisourceHttpProvider,
    private val key: String
) : Catalog {

    private fun builderForRequest(request: String) =
        ApiUriBuilder.forQuery(key).setRequest(request)

    override fun queryAuthors(visitor: AuthorsVisitor, progress: ProgressCallback) {
        LOG.d { "queryAuthors()" }
        val uri = builderForRequest("search_artist").build()
        val root = createAuthorsParserRoot(visitor)
        loadPages(uri, root, progress)
        visitor.accept(Author.UNKNOWN)
    }

    override fun queryGenres(visitor: GenresVisitor) {
        LOG.d { "queryGenres()" }
        val uri = builderForRequest("view_genres").build()
        val root = createGenresParserRoot(visitor)
        loadSinglePage(uri, root)
    }

    override fun queryTracks(
        author: Author,
        visitor: TracksVisitor,
        progress: ProgressCallback
    ) {
        LOG.d { "queryTracks(author=${author.id})" }
        val uri = builderForRequest("view_modules_by_artistid").setQuery(author.id).build()
        val root = createTracksParserRoot(visitor)
        loadPages(uri, root, progress)
    }

    override fun queryTracks(
        genre: Genre,
        visitor: TracksVisitor,
        progress: ProgressCallback
    ) {
        LOG.d { "queryTracks(genre=${genre.id})" }
        val uri = builderForRequest("search").setType("genre").setQuery(genre.id).build()
        val root = createTracksParserRoot(visitor)
        loadPages(uri, root, progress)
    }

    open fun searchSupported() = http.hasConnection()

    override fun findTracks(query: String, visitor: FoundTracksVisitor) {
        LOG.d { "findTracks(query=$query)" }
        val uri = builderForRequest("search").setType("filename_or_songtitle").setQuery("*$query*")
            .build()
        val root = createFoundTracksParserRoot(visitor)
        loadPages(uri, root, StubProgressCallback.instance())
    }

    override fun findRandomTracks(visitor: TracksVisitor) {
        LOG.d { "findRandomTracks()" }
        val uri = builderForRequest("random").build()
        val root = createTracksParserRoot(visitor)
        loadSinglePage(uri, root)
    }

    private fun loadPages(baseUri: String, root: RootElement, progress: ProgressCallback) {
        var totalPages = 1
        root.getChild("totalpages").setEndTextElementListener { body: String? ->
            if (totalPages == 1) {
                tryGetInteger(body)?.let { result ->
                    LOG.d { "Loading $result pages" }
                    totalPages = result
                }
            }
        }
        var page = 1
        while (page <= totalPages) {
            val pageUri = "$baseUri&page=$page"
            loadSinglePage(pageUri, root)
            progress.onProgressUpdate(page - 1, totalPages)
            ++page
        }
    }

    private fun loadSinglePage(uri: String, root: RootElement) =
        http.getInputStream(Uri.parse(uri)).let { httpStream ->
            Xml.parse(BufferedInputStream(httpStream), root.contentHandler)
        }

    companion object {
        @JvmStatic
        fun getTrackUris(id: Int) = arrayOf(
            Cdn.modarchive(id),
            ApiUriBuilder.forDownload(id).buildUri()
        )
    }
}

private class ApiUriBuilder {

    private val delegate: Uri.Builder = Uri.Builder()

    init {
        delegate.scheme("https")
        delegate.authority("api.modarchive.org")
    }

    fun setRequest(request: String) = apply {
        delegate.appendQueryParameter("request", request)
    }

    fun setQuery(query: String) = apply {
        delegate.appendQueryParameter("query", query)
    }

    fun setQuery(query: Int) = setQuery(query.toString())

    fun setType(type: String) = apply {
        delegate.appendQueryParameter("type", type)
    }

    fun buildUri(): Uri = delegate.build()

    fun build() = buildUri().toString()

    companion object {
        fun forQuery(key: String) = ApiUriBuilder().apply {
            delegate.path("xml-tools.php")
            delegate.appendQueryParameter("key", key)
        }

        fun forDownload(trackId: Int) = ApiUriBuilder().apply {
            delegate.path("downloads.php")
            delegate.appendQueryParameter("moduleid", trackId.toString())
        }
    }
}

private class AuthorBuilder {
    private var id: Int? = null
    private var alias: String? = null

    fun setId(value: String?) {
        id = tryGetInteger(value)
    }

    fun setAlias(value: String?) {
        alias = value
    }

    fun captureResult() = ifNotNulls(id, alias, ::Author).also {
        id = null
        alias = null
    }
}

private class GenreBuilder {
    private var id: Int? = null
    private var text: String? = null
    private var files: Int? = null

    fun setId(value: String?) {
        id = tryGetInteger(value)
    }

    fun setText(value: String?) {
        text = value
    }

    fun setFiles(value: String?) {
        files = tryGetInteger(value)
    }

    fun captureResult() = ifNotNulls(id, text, files, ::Genre).also {
        id = null
        files = null
        text = null
    }
}

private class TrackBuilder {
    private var id: Int? = null
    private var filename: String? = null
    private var title: String? = null
    private var size: Int? = null

    fun setId(value: String?) {
        id = tryGetInteger(value)
    }

    fun setFilename(value: String?) {
        filename = value
    }

    // CDATA
    fun setTitle(value: String?) {
        title = value?.let { Html.fromHtml(it).toString() }
    }

    fun setSize(value: String?) {
        size = tryGetInteger(value)
    }

    fun captureResult() = ifNotNulls(id, filename, title, size, ::Track).also {
        id = null
        size = null
        filename = null
        title = null
    }
}

private fun createAuthorsParserRoot(visitor: AuthorsVisitor) =
    createRootElement().apply {
        getChild("total_results").setEndTextElementListener { body: String? ->
            tryGetInteger(body)?.let { result ->
                visitor.setCountHint(result)
            }
        }
        val builder = AuthorBuilder()
        getChild("items").getChild("item").run {
            setEndElementListener {
                builder.captureResult()?.let {
                    visitor.accept(it)
                }
            }
            bindAuthorFields(this, builder)
        }
    }

private fun createGenresParserRoot(visitor: GenresVisitor) = createRootElement().apply {
    getChild("results").setEndTextElementListener { body: String? ->
        tryGetInteger(body)?.let { result ->
            visitor.setCountHint(result)
        }
    }
    val builder = GenreBuilder()
    getChild("parent").getChild("children").getChild("child").run {
        setEndElementListener {
            builder.captureResult()?.let {
                visitor.accept(it)
            }
        }
        getChild("id").setEndTextElementListener(builder::setId)
        getChild("text").setEndTextElementListener(builder::setText)
        getChild("files").setEndTextElementListener(builder::setFiles)
    }
}

private fun createTracksParserRoot(visitor: TracksVisitor) = createRootElement().apply {
    getChild("total_results").setEndTextElementListener { body: String? ->
        tryGetInteger(body)?.let { result ->
            visitor.setCountHint(result)
        }
    }
    val builder = TrackBuilder()
    getChild("module").run {
        setEndElementListener {
            builder.captureResult()?.let {
                visitor.accept(it)
            }
        }
        bindTrackFields(this, builder)
    }
}

private fun createFoundTracksParserRoot(visitor: FoundTracksVisitor) = createRootElement().apply {
    getChild("results").setEndTextElementListener { body: String? ->
        tryGetInteger(body)?.let { result ->
            visitor.setCountHint(result)
        }
    }
    val trackBuilder = TrackBuilder()
    val authorBuilder = AuthorBuilder()
    getChild("module").run {
        setEndElementListener {
            val author = authorBuilder.captureResult()
            trackBuilder.captureResult()?.let { track ->
                visitor.accept(author ?: Author.UNKNOWN, track)
            }
        }
        bindTrackFields(this, trackBuilder)
        bindAuthorFields(getChild("artist_info").getChild("artist"), authorBuilder)
    }
}

private fun bindTrackFields(item: Element, builder: TrackBuilder) = with(item) {
    getChild("id").setEndTextElementListener(builder::setId)
    getChild("filename").setEndTextElementListener(builder::setFilename)
    getChild("bytes").setEndTextElementListener(builder::setSize)
    getChild("songtitle").setEndTextElementListener(builder::setTitle)
}

private fun bindAuthorFields(item: Element, builder: AuthorBuilder) = with(item) {
    getChild("id").setEndTextElementListener(builder::setId)
    getChild("alias").setEndTextElementListener(builder::setAlias)
}

private fun createRootElement() = RootElement("modarchive")
